/*
 * Copyright (C) 2020 Jordan Hendl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* 
 * File:   CommandBuffer.cpp
 * Author: Jordan Hendl
 * 
 * Created on December 30, 2020, 1:42 PM
 */

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_ASSERT_ON_RESULT
#define VULKAN_HPP_NOEXCEPT
#define VULKAN_HPP_NOEXCEPT_WHEN_NO_EXCEPTIONS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "CommandBuffer.h"
#include "Queue.h"
#include "Pipeline.h"
#include "Vulkan.h"
#include "Device.h"
#include "RenderPass.h"
#include "Descriptor.h"
#include <algorithm>
#include <vulkan/vulkan.hpp>
#include <map>
#include <vector>
#include <mutex>
#include <thread>
#include <algorithm>

namespace nyx
{
  namespace vkg
  {
    typedef unsigned Family ;
    using PoolMap   = std::unordered_map<Family, vk::CommandPool>  ;
    using MutexMap  = std::unordered_map<Family, std::mutex     >  ;
    using ThreadMap = std::unordered_map<std::thread::id, PoolMap> ;
    static ThreadMap thread_map ;
    static MutexMap mutex_map ;

    struct CommandBufferData
    {
      using CmdBuffers = std::vector<vk::CommandBuffer> ;
      using Fences     = std::vector<vk::Fence>         ;
      
      std::mutex*                      pool_mutex          ;
      vk::CommandBufferInheritanceInfo inheritance         ;
      mutable vk::SubpassContents      subpass_flags       ;
      vk::PipelineBindPoint            bind_point          ;
      vk::Device                       device              ;
      unsigned                         id                  ;
      nyx::vkg::Queue                  queue               ;
      vk::Pipeline                     pipeline            ;
      vk::PipelineLayout               pipeline_layout     ;
      vk::CommandBufferBeginInfo       begin_info          ;
      vk::CommandPool                  vk_pool             ;
      Fences                           fences              ;
      CommandBuffer::Level             level               ;
      CmdBuffers                       cmd_buffers         ;
      mutable std::vector<bool>        is_signaled         ;
      std::vector<bool>                started_render_pass ;
      bool                             recording           ;
      mutable unsigned                 current             ;
      
      /** Default constructor.
       */
      CommandBufferData() ;
      
      /** Method to retrieve the map of command pools from a queue family.
       * @note creates pool if it is not found.
       * @param queue_family
       * @return A Reference to the created Command Pool.
       */
      vk::CommandPool& pool( Family queue_family ) ;
      
      /** Method to retrieve the map of command pools from a queue family.
       * @note creates pool if it is not found.
       * @param queue_family
       * @return A Reference to the mutex of the family.
       */
      std::mutex* mutex( Family queue_family ) ;
    };
    
    
    CommandBufferData::CommandBufferData()
    {
      this->pool_mutex          = nullptr                       ;
      this->subpass_flags       = vk::SubpassContents::eInline  ;
      this->pipeline            = nullptr                       ;
      this->pipeline_layout     = nullptr                       ;
      this->level               = CommandBuffer::Level::Primary ;
      this->recording           = false                         ;
      this->current             = 0                             ;
    }
    
    std::mutex* CommandBufferData::mutex( Family queue_family )
    {
      return &mutex_map[ queue_family ] ;
    }
    
    vk::CommandPool& CommandBufferData::pool( Family queue_family )
    {
      const auto id = std::this_thread::get_id() ;
      const PoolMap::iterator          iter   = thread_map[ id ].find( queue_family ) ;
      const vk::CommandPoolCreateFlags flags  = vk::CommandPoolCreateFlagBits::eResetCommandBuffer ; // TODO make this configurable.
      const vk::Device                 device = this->device                                       ;

      vk::CommandPoolCreateInfo info ;
      vk::CommandPool           pool ;
      
      if( iter == thread_map[ id ].end() )
      {
        info.setFlags           ( flags        ) ;
        info.setQueueFamilyIndex( queue_family ) ;
        
        return thread_map[ id ].insert( iter, { queue_family, device.createCommandPool( info, nullptr ) } )->second ;
      }
      
      return iter->second ;
    }

    CommandBuffer::CommandBuffer()
    {
      this->cmd_data = new CommandBufferData() ;
    }

    CommandBuffer::CommandBuffer( const CommandBuffer& cmd )
    {
      this->cmd_data = new CommandBufferData() ;
      *this = cmd ;
    }

    CommandBuffer::~CommandBuffer()
    {
      delete this->cmd_data ;
    }

    CommandBuffer& CommandBuffer::operator=( const CommandBuffer& cmd )
    {
      *this->cmd_data = *cmd.cmd_data ;
      
      return *this ;
    }
    
    void CommandBuffer::barrier( const vkg::Buffer& read, const vkg::Buffer& write )
    {
      vk::BufferMemoryBarrier barrier ;
      
      write.size() ;
      barrier.setBuffer       ( read.buffer()                    ) ;
      barrier.setSize         ( VK_WHOLE_SIZE                    ) ;
      barrier.setSrcAccessMask( vk::AccessFlagBits::eMemoryWrite ) ;
      barrier.setDstAccessMask( vk::AccessFlagBits::eMemoryRead  ) ;
      data().cmd_buffers[ data().current ].pipelineBarrier( vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
                                                            vk::DependencyFlags(), nullptr, barrier, nullptr ) ;
    }
    
    void CommandBuffer::barrier( const vkg::Buffer& read, const vkg::Image& write )
    {
      vk::BufferMemoryBarrier barrier ;
      
      write.size() ;
      barrier.setBuffer       ( read.buffer()                    ) ;
      barrier.setSize         ( VK_WHOLE_SIZE                    ) ;
      barrier.setSrcAccessMask( vk::AccessFlagBits::eMemoryWrite ) ;
      barrier.setDstAccessMask( vk::AccessFlagBits::eMemoryRead  ) ;
      data().cmd_buffers[ data().current ].pipelineBarrier( vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
                                                            vk::DependencyFlags(), nullptr, barrier, nullptr ) ;
    }
    
    void CommandBuffer::bind( const nyx::vkg::Descriptor& descriptor )
    {
      if( descriptor.set() ) 
      {
        data().cmd_buffers[ data().current ].bindDescriptorSets( data().bind_point, data().pipeline_layout, 0, 1, &descriptor.set(), 0, nullptr ) ;
      }
    }

    void CommandBuffer::bind( const nyx::vkg::Pipeline& pipeline )
    {
      data().bind_point      = pipeline.isGraphics() ? vk::PipelineBindPoint::eGraphics : vk::PipelineBindPoint::eCompute ;
      data().pipeline        = pipeline.pipeline()                                                                        ;
      data().pipeline_layout = pipeline.layout()                                                                          ;
      
      data().cmd_buffers[ data().current ].bindPipeline( data().bind_point, data().pipeline ) ;
    }
    
    void CommandBuffer::pushConstantBase( const void* value, unsigned byte_size, unsigned offset )
    {
      const auto flags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute ;
      char buff[ 256 ] ;
     
      if( byte_size < 256 )
      {
        std::memcpy( buff, reinterpret_cast<const char*>( ( value ) ), byte_size ) ;
        data().cmd_buffers[ data().current ].pushConstants( data().pipeline_layout, flags, offset, 256 - offset, buff ) ;
      }
    }
    
    void CommandBuffer::initialize( const CommandBuffer& parent ) 
    {
      vk::CommandBufferAllocateInfo info       ;
      vk::CommandBufferLevel        cmd_level  ;
      vk::CommandPool               pool       ;
      vk::Device                    device     ;
      vk::FenceCreateInfo           fence_info ;
      
      fence_info.setFlags( vk::FenceCreateFlagBits::eSignaled ) ;
      
      data().queue = parent.data().queue ;
      data().level = Level::Secondary ;

      Vulkan::initialize() ;
      data().device = Vulkan::device( parent.data().queue.device() ).device() ;
      data().id     = data().queue.device() ;

      device            = data().device                         ;
      pool              = data().pool( data().queue.family() )  ;
      data().vk_pool    = data().pool( data().queue.family() )  ;
      data().pool_mutex = data().mutex( data().queue.family() ) ;
      cmd_level         = vk::CommandBufferLevel::eSecondary    ;
      
      info.setCommandBufferCount( parent.data().cmd_buffers.size() ) ;
      info.setLevel             ( cmd_level                        ) ;
      info.setCommandPool       ( pool                             ) ;
      
      data().cmd_buffers        .resize( parent.data().cmd_buffers.size() ) ;
      data().fences             .resize( parent.data().cmd_buffers.size() ) ;
      data().started_render_pass.resize( parent.data().cmd_buffers.size() ) ;
      data().is_signaled        .resize( parent.data().cmd_buffers.size() ) ;
      
      std::fill( data().started_render_pass.begin(), data().started_render_pass.end(), false ) ;
      std::fill( data().is_signaled        .begin(), data().is_signaled        .end(), true  ) ;
      
      vkg::Vulkan::add( device.allocateCommandBuffers( &info, data().cmd_buffers.data() ) ) ;
      
      data().subpass_flags = vk::SubpassContents::eSecondaryCommandBuffers ;
      
      for( auto& fence : data().fences )
      {
        auto result = device.createFence( fence_info ) ;
        vkg::Vulkan::add( result.result ) ;
        fence = result.value ;
      }
    }

    void CommandBuffer::initialize( const nyx::vkg::Queue& queue, unsigned count, CommandBuffer::Level level, bool multi_pass ) 
    {
      vk::CommandBufferAllocateInfo info       ;
      vk::CommandBufferLevel        cmd_level  ;
      vk::CommandPool               pool       ;
      vk::Device                    device     ;
      vk::FenceCreateInfo           fence_info ;
      
      fence_info.setFlags( vk::FenceCreateFlagBits::eSignaled ) ;
      
      data().queue = queue ;
      data().level = level ;

      Vulkan::initialize() ;
      data().device = Vulkan::device( queue.device() ).device() ;
      data().id     = queue.device()                            ;

      device            = data().device                         ;
      pool              = data().pool( data().queue.family() )  ;
      data().vk_pool    = data().pool( data().queue.family() )  ;
      data().pool_mutex = data().mutex( data().queue.family() ) ;

      cmd_level      = level == Level::Primary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary ;
      
      info.setCommandBufferCount( count     ) ;
      info.setLevel             ( cmd_level ) ;
      info.setCommandPool       ( pool      ) ;
      
      data().cmd_buffers        .resize( count ) ;
      data().fences             .resize( count ) ;
      data().started_render_pass.resize( count ) ;
      data().is_signaled        .resize( count ) ;

      data().subpass_flags = multi_pass ? vk::SubpassContents::eSecondaryCommandBuffers : vk::SubpassContents::eInline ;

      std::fill( data().started_render_pass.begin(), data().started_render_pass.end(), false ) ;
      std::fill( data().is_signaled        .begin(), data().is_signaled        .end(), true  ) ;
      vkg::Vulkan::add( device.allocateCommandBuffers( &info, data().cmd_buffers.data() ) ) ;
      
      for( auto& fence : data().fences )
      {
        auto result = device.createFence( fence_info ) ;
        vkg::Vulkan::add( result.result ) ;
        fence = result.value ;
      }
    }
    
    bool CommandBuffer::initialized() const
    {
      return !data().cmd_buffers.empty() ;
    }

    void CommandBuffer::nextSubpass()
    {
      if( data().level == Level::Primary )
      {
        if( data().started_render_pass[ data().current ] ) data().cmd_buffers[ data().current ].nextSubpass( data().subpass_flags ) ;
      }
    }
    
    void CommandBuffer::combine( const CommandBuffer& cmd )
    {
      if( data().level == Level::Primary && cmd.data().level == Level::Secondary && data().subpass_flags == vk::SubpassContents::eSecondaryCommandBuffers )
      {
        data().cmd_buffers[ data().current ].executeCommands( 1, &cmd.data().cmd_buffers[ cmd.data().current ] ) ;
      }
    }
    
    vk::Fence CommandBuffer::fence() const
    {
      data().is_signaled[ data().current ] = true ;
      return data().fences[ data().current ] ;
    }
    
    void CommandBuffer::advance() const
    {
      data().current++ ;
      if( data().current >= data().cmd_buffers.size() ) data().current = 0 ;
    }
    
    unsigned CommandBuffer::current() const
    {
      return data().current ;
    }

    unsigned CommandBuffer::size() const
    {
      return data().cmd_buffers.size() ;
    }

    const nyx::vkg::Queue& CommandBuffer::queue() const
    {
      return data().queue ;
    }

    unsigned CommandBuffer::device() const
    {
      return data().id ;
    }
    
    CommandBuffer::Level CommandBuffer::level() const
    {
      return data().level ;
    }

    const vk::CommandBuffer& CommandBuffer::buffer() const
    {
      return data().cmd_buffers[ data().current ] ;
    }
    
    const vk::CommandBuffer* CommandBuffer::pointer() const
    {
      return data().cmd_buffers.data() ;
    }

    void CommandBuffer::drawBase( const nyx::vkg::Buffer& buffer, unsigned count, unsigned offset )
    {
      const vk::DeviceSize device_size = offset ;
      
      data().cmd_buffers[ data().current ].bindVertexBuffers( 0, 1, &buffer.buffer(), &device_size ) ;
      data().cmd_buffers[ data().current ].draw( count, 1, 0, 0 ) ;
    }
    
    void CommandBuffer::drawIndexedBase( const nyx::vkg::Buffer& index, const nyx::vkg::Buffer& vert, unsigned index_count, unsigned vert_count, unsigned offset ) 
    {
      const vk::DeviceSize device_size = offset                 ;
      const vk::IndexType  type        = vk::IndexType::eUint32 ;

      vert_count = vert_count ;
      data().cmd_buffers[ data().current ].bindVertexBuffers( 0, 1, &vert.buffer(), &device_size ) ;
      data().cmd_buffers[ data().current ].bindIndexBuffer  ( index.buffer(), 0, type            ) ;
      data().cmd_buffers[ data().current ].drawIndexed      ( index_count, 1, 0, 0, 0            ) ;
    }

    void CommandBuffer::drawInstanced( const nyx::vkg::Buffer& vertices, unsigned vert_count, unsigned instance_count, unsigned offset, unsigned first )
    {
      const vk::DeviceSize device_size = offset ;
      
      data().cmd_buffers[ data().current ].bindVertexBuffers( 0, 1, &vertices.buffer(), &device_size ) ;
      data().cmd_buffers[ data().current ].draw( vert_count, instance_count, offset, first ) ;
    }
    
    void CommandBuffer::drawInstanced( const nyx::vkg::Buffer& indices, unsigned index_count, const nyx::vkg::Buffer& vertices, unsigned vert_count, unsigned instance_count, unsigned offset, unsigned first )
    {
      const vk::DeviceSize device_size = offset                 ;
      const vk::IndexType  type        = vk::IndexType::eUint32 ;

      vert_count = vert_count ;
      data().cmd_buffers[ data().current ].bindVertexBuffers( 0, 1, &vertices.buffer(), &device_size        ) ;
      data().cmd_buffers[ data().current ].bindIndexBuffer  ( indices.buffer(), 0, type                     ) ;
      data().cmd_buffers[ data().current ].drawIndexed      ( index_count, instance_count, offset, 0, first ) ;
    }
    
    bool CommandBuffer::recording() const
    {
      return data().recording ;
    }

    void CommandBuffer::record( const nyx::vkg::RenderPass& render_pass, unsigned index )
    {
      vk::Fence               fence ;
      vk::RenderPassBeginInfo info  ;
      
      info.setClearValueCount( render_pass.count()       ) ;
      info.setPClearValues   ( render_pass.clearValues() ) ;
      info.setRenderArea     ( render_pass.area()        ) ;
      info.setRenderPass     ( render_pass.pass()        ) ;
      info.setFramebuffer    ( render_pass.current()     ) ;
      
      fence = data().fences[ data().current ] ;
      
      data().pool_mutex->lock() ;
      if( data().level == Level::Primary )
      {
        if( data().is_signaled[ data().current ] )
        {
          vkg::Vulkan::add( data().device.waitForFences( 1, &fence, true, UINT64_MAX ) ) ;
          vkg::Vulkan::add( data().device.resetFences( 1, &fence )                     ) ;
          data().is_signaled[ data().current ] = false ;
        }
        
        vkg::Vulkan::add( data().cmd_buffers[ data().current ].begin          ( &data().begin_info )        ) ;
                          data().cmd_buffers[ data().current ].beginRenderPass( &info, data().subpass_flags ) ;
  
        data().recording                             = true ;
        data().started_render_pass[ data().current ] = true ;
      }
      else
      {
//        data().inheritance.setFramebuffer    ( render_pass.current()                               ) ;
        data().inheritance.setSubpass        ( index                                               ) ;
        data().inheritance.setRenderPass     ( render_pass.pass()                                  ) ;
        data().begin_info.setPInheritanceInfo( &data().inheritance                                 ) ;
        data().begin_info.setFlags           ( vk::CommandBufferUsageFlagBits::eRenderPassContinue ) ;

        vkg::Vulkan::add( data().cmd_buffers[ data().current ].begin( &data().begin_info ) ) ;
        
        data().recording = true ;
      }
      data().pool_mutex->unlock() ;
    }

    void CommandBuffer::record( const nyx::vkg::RenderPass& render_pass )
    {
      vk::RenderPassBeginInfo info  ;
      vk::Fence               fence ;
      
      info.setClearValueCount( render_pass.count()       ) ;
      info.setPClearValues   ( render_pass.clearValues() ) ;
      info.setRenderArea     ( render_pass.area()        ) ;
      info.setRenderPass     ( render_pass.pass()        ) ;
      info.setFramebuffer    ( render_pass.current()     ) ;
      
      fence = data().fences[ data().current ] ;
      
      data().pool_mutex->lock() ;
      if( data().level == Level::Primary )
      {
        if( data().is_signaled[ data().current ] )
        {
          vkg::Vulkan::add( data().device.waitForFences( 1, &fence, true, UINT64_MAX ) ) ;
          vkg::Vulkan::add( data().device.resetFences( 1, &fence )                     ) ;
          data().is_signaled[ data().current ] = false ;
        }
  
        vkg::Vulkan::add( data().cmd_buffers[ data().current ].begin          ( &data().begin_info )        ) ;
                          data().cmd_buffers[ data().current ].beginRenderPass( &info, data().subpass_flags ) ;
        
        data().recording                             = true ;
        data().started_render_pass[ data().current ] = true ;
      }
      data().pool_mutex->unlock() ;
    }

    void CommandBuffer::record()
    {
      vk::Fence fence = data().fences[ data().current ] ;
      
      data().pool_mutex->lock() ;
      if( data().is_signaled[ data().current ] )
      {
        vkg::Vulkan::add( data().device.waitForFences( 1, &fence, true, UINT64_MAX ) ) ;
        vkg::Vulkan::add( data().device.resetFences( 1, &fence )                     ) ;
        data().is_signaled[ data().current ] = false ;
      }

      vkg::Vulkan::add( data().cmd_buffers[ data().current ].begin( &data().begin_info ) ) ;
      data().recording = true ;
      data().pool_mutex->unlock() ;
    }
    
    void CommandBuffer::setActive( unsigned index )
    {
      data().current = index < data().cmd_buffers.size() ? index : data().current ;
    }

    void CommandBuffer::stop()
    {
      data().recording = false ;
      vk::CommandBuffer cmd_buff = data().cmd_buffers[ data().current ] ;
      
      data().pool_mutex->lock() ;
      if( data().started_render_pass[ data().current ] )
      {
        cmd_buff.endRenderPass() ;
      }
      
      vkg::Vulkan::add( cmd_buff.end() ) ;
      data().started_render_pass[ data().current ] = false ;
      data().pool_mutex->unlock() ;
    }

    void CommandBuffer::reset()
    {
      const vk::Device device = data().device ;
      
      if( data().cmd_buffers.size() != 0 ) device.freeCommandBuffers( data().vk_pool, data().cmd_buffers.size(), data().cmd_buffers.data() ) ;
      data().cmd_buffers.clear() ;
    }

    CommandBufferData& CommandBuffer::data()
    {
      return *this->cmd_data ;
    }

    const CommandBufferData& CommandBuffer::data() const
    {
      return *this->cmd_data ;
    }
  }
}
