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
 * File:   Pipeline.cpp
 * Author: Jordan Hendl
 * 
 * Created on December 30, 2020, 1:21 AM
 */

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_ASSERT_ON_RESULT
#define VULKAN_HPP_NOEXCEPT

#include "Pipeline.h"
#include "Device.h"
#include "NyxShader.h"
#include "RenderPass.h"
#include "Vulkan.h"
#include <vulkan/vulkan.hpp>

namespace nyx
{
  namespace vkg
  {
    /** The structure to contain all pipeline configuration.
     */
    struct PipelineConfig
    {
      vk::Viewport                             viewport               ; ///< TODO
      vk::Rect2D                               scissor                ; ///< TODO
      vk::PipelineViewportStateCreateInfo      viewport_info          ; ///< TODO
      vk::PipelineColorBlendStateCreateInfo    color_blend_info       ; ///< TODO
      vk::PipelineRasterizationStateCreateInfo rasterization_info     ; ///< TODO
      vk::PipelineInputAssemblyStateCreateInfo assembly_info          ; ///< TODO
      vk::PipelineMultisampleStateCreateInfo   multisample_info       ; ///< TODO
      vk::PipelineColorBlendAttachmentState    color_blend_attachment ; ///< TODO
      
      /** Default constructor.
       */
      PipelineConfig() ;
    };

    /** The structure to contain the underlying pipeline data.
     */
    struct PipelineData
    {
      PipelineConfig       config              ; ///< TODO
      nyx::vkg::Device     device              ; ///< TODO
      nyx::vkg::RenderPass render_pass         ; ///< TODO
      nyx::vkg::NyxShader   shader              ; ///< TODO
      vk::Pipeline         pipeline            ; ///< TODO
      vk::PipelineLayout   layout              ; ///< TODO
      vk::PipelineCache    cache               ; ///< TODO
      vk::ShaderStageFlags push_constant_flags ; ///< TODO
      unsigned             push_constant_size  ; ///< TODO
      
      /** Default constructor.
       */
      PipelineData() ;
      
      /** Helper method to create a vulkan pipeline layout.
       */
      void createLayout() ;
      
      /** Helper method to create a vulkan pipeline.
       */
      void createPipeline() ;
    };
    
    PipelineConfig::PipelineConfig()
    {
      const ::vk::ColorComponentFlags color_blend_mask = ::vk::ColorComponentFlagBits::eR | ::vk::ColorComponentFlagBits::eG | ::vk::ColorComponentFlagBits::eB | ::vk::ColorComponentFlagBits::eA ;
      
      this->rasterization_info.setDepthClampEnable       ( false                         ) ;
      this->rasterization_info.setRasterizerDiscardEnable( false                         ) ;
      this->rasterization_info.setPolygonMode            ( ::vk::PolygonMode::eFill      ) ;
      this->rasterization_info.setLineWidth              ( 1.0f                          ) ;
      this->rasterization_info.setCullMode               ( ::vk::CullModeFlagBits::eNone ) ;
      this->rasterization_info.setFrontFace              ( ::vk::FrontFace::eClockwise   ) ;
      this->rasterization_info.setDepthBiasEnable        ( false                         ) ;
      this->rasterization_info.setDepthBiasConstantFactor( 0.0f                          ) ;
      this->rasterization_info.setDepthBiasClamp         ( 0.0f                          ) ;
      this->rasterization_info.setDepthBiasSlopeFactor   ( 0.0f                          ) ;
      
      this->multisample_info.setSampleShadingEnable  ( false                         ) ;
      this->multisample_info.setMinSampleShading     ( 1.0f                          ) ;
      this->multisample_info.setAlphaToOneEnable     ( false                         ) ;
      this->multisample_info.setAlphaToCoverageEnable( false                         ) ;
      this->multisample_info.setPSampleMask          ( nullptr                       ) ;
      this->multisample_info.setRasterizationSamples ( ::vk::SampleCountFlagBits::e1 ) ;
      
      this->scissor.offset.setX( 0 ) ;
      this->scissor.offset.setY( 0 ) ;
      
      this->color_blend_attachment.setColorWriteMask     ( color_blend_mask             ) ;
      this->color_blend_attachment.setBlendEnable        ( false                        ) ;
      this->color_blend_attachment.setSrcColorBlendFactor( ::vk::BlendFactor::eSrcAlpha ) ;
      this->color_blend_attachment.setDstColorBlendFactor( ::vk::BlendFactor::eOne      ) ;
      this->color_blend_attachment.setColorBlendOp       ( ::vk::BlendOp::eAdd          ) ;
      this->color_blend_attachment.setSrcAlphaBlendFactor( ::vk::BlendFactor::eOne      ) ;
      this->color_blend_attachment.setDstAlphaBlendFactor( ::vk::BlendFactor::eZero     ) ;
      this->color_blend_attachment.setAlphaBlendOp       ( ::vk::BlendOp::eAdd          ) ;
      
      this->color_blend_info.setLogicOpEnable    ( false                         ) ;
      this->color_blend_info.setLogicOp          ( ::vk::LogicOp::eCopy          ) ;
      this->color_blend_info.setAttachmentCount  ( 1                             ) ;
      this->color_blend_info.setPAttachments     ( &this->color_blend_attachment ) ;
      
      this->viewport_info.setViewportCount( 1 ) ;
      this->viewport_info.setScissorCount ( 1 ) ;

      this->assembly_info.setTopology              ( ::vk::PrimitiveTopology::eTriangleList ) ;
      this->assembly_info.setPrimitiveRestartEnable( false                                  ) ;
    }

    PipelineData::PipelineData()
    {
      this->push_constant_size  = 256                              ;
      this->push_constant_flags = vk::ShaderStageFlagBits::eVertex ;
    }

    void PipelineData::createLayout()
    {
      vk::PipelineLayoutCreateInfo info        ;
      vk::PushConstantRange        range       ;
      vk::DescriptorSetLayout      desc_layout ;
      
      desc_layout = this->shader.layout() ;
      
      range.setOffset    ( 0                         ) ;
      range.setSize      ( this->push_constant_size  ) ;
      range.setStageFlags( this->push_constant_flags ) ;
      
      info.setSetLayoutCount        ( 1                                     ) ;
      info.setPSetLayouts           ( &desc_layout                          ) ;
      info.setPushConstantRangeCount( 1                                     ) ;
      info.setPPushConstantRanges   ( &range                                ) ;
      
      auto result = this->device.device().createPipelineLayout( info, nullptr ) ;
      vkg::Vulkan::add( result.result ) ;
      this->layout = result.value ;
    }
    
    void PipelineData::createPipeline()
    {
      vk::GraphicsPipelineCreateInfo         graphics_info ;
      vk::ComputePipelineCreateInfo          compute_info  ;
      vk::PipelineVertexInputStateCreateInfo vertex_input  ;
      
      vertex_input.setVertexAttributeDescriptionCount( this->shader.numVertexAttributes() ) ;
      vertex_input.setPVertexAttributeDescriptions   ( this->shader.attributes()          ) ;
      vertex_input.setVertexBindingDescriptionCount  ( this->shader.numVertexBindings()   ) ;
      vertex_input.setPVertexBindingDescriptions     ( this->shader.bindings()            ) ;
      

      if( this->render_pass.initialized() )
      {
        this->config.viewport_info.setPViewports( this->render_pass.viewports() ) ;
        this->config.viewport_info.setPScissors ( this->render_pass.scissors()  ) ;

        graphics_info.setPStages            ( this->shader.infos()             ) ;
        graphics_info.setStageCount         ( this->shader.numStages()         ) ;
        graphics_info.setLayout             ( this->layout                     ) ;
        graphics_info.setPVertexInputState  ( &vertex_input                    ) ;
        graphics_info.setPInputAssemblyState( &this->config.assembly_info      ) ;
        graphics_info.setPViewportState     ( &this->config.viewport_info      ) ;
        graphics_info.setPRasterizationState( &this->config.rasterization_info ) ;
        graphics_info.setPMultisampleState  ( &this->config.multisample_info   ) ;
        graphics_info.setPColorBlendState   ( &this->config.color_blend_info   ) ;
        graphics_info.setRenderPass         ( this->render_pass.pass()         ) ;
        
        auto result = this->device.device().createGraphicsPipeline( this->cache, graphics_info ) ;
        vkg::Vulkan::add( result.result ) ;
        this->pipeline = result.value ;
      }
      else
      {
        compute_info.setLayout( this->layout              ) ;
        compute_info.setStage ( this->shader.infos()[ 0 ] ) ;
        
        auto result = ( this->device.device().createComputePipeline( this->cache, compute_info ) ) ;
        vkg::Vulkan::add( result.result ) ;
        this->pipeline = result.value ;
      }
    }
    
    Pipeline::Pipeline()
    {
      this->pipeline_data = new PipelineData() ;
    }

    Pipeline::Pipeline( const Pipeline& pipeline )
    {
      this->pipeline_data = new PipelineData() ;
    
      *this = pipeline ;
    }

    Pipeline::~Pipeline()
    {
      delete this->pipeline_data ;
    }

    Pipeline& Pipeline::operator=( const Pipeline& pipeline )
    {
      *this->pipeline_data = *pipeline.pipeline_data ;
      
      return *this ;
    }

    Pipeline::operator ::vk::Pipeline&()
    {
      return data().pipeline ;
    }

    Pipeline::operator const ::vk::Pipeline&()
    {
      return data().pipeline ;
    }

    void Pipeline::initialize( unsigned device, const char* nyx_file )
    {
      Vulkan::initialize() ;

      data().device = Vulkan::device( device ) ;
      data().shader.initialize( device, nyx_file ) ;
      
      data().createLayout() ;
      data().createPipeline() ;
    }

    void Pipeline::initialize( const nyx::vkg::RenderPass& pass, const char* nyx_file )
    {
      if( !Vulkan::initialized() ) Vulkan::initialize() ;

      data().render_pass = pass                            ;
      data().device      = Vulkan::device( pass.device() ) ;

      data().shader.initialize( pass.device(), nyx_file ) ;
      
      data().createLayout() ;
      data().createPipeline() ;
    }
    
    void Pipeline::initialize( unsigned device, const unsigned char* nyx_bytes, unsigned size )
    {
      Vulkan::initialize() ;

      data().device = Vulkan::device( device ) ;
      data().shader.initialize( device, nyx_bytes, size ) ;
      
      data().createLayout() ;
      data().createPipeline() ;
    }

    void Pipeline::initialize( const nyx::vkg::RenderPass& pass, const unsigned char* nyx_bytes, unsigned size )
    {
      if( !Vulkan::initialized() ) Vulkan::initialize() ;

      data().render_pass = pass                            ;
      data().device      = Vulkan::device( pass.device() ) ;

      data().shader.initialize( pass.device(), nyx_bytes, size ) ;
      
      data().createLayout() ;
      data().createPipeline() ;
    }
    
    bool Pipeline::isGraphics() const
    {
      return data().render_pass.initialized() ;
    }

    void Pipeline::initialize( const NyxShader& shader )
    {
      Vulkan::initialize() ;

      data().shader = shader                            ;
      data().device = Vulkan::device( shader.device() ) ;
      
      data().createLayout() ;
      data().createPipeline() ;
    }

    void Pipeline::initialize( const nyx::vkg::RenderPass& pass, const NyxShader& shader )
    {
      Vulkan::initialize() ;

      data().render_pass = pass                            ;
      data().device      = Vulkan::device( pass.device() ) ;
      data().shader      = shader                          ;
      
      data().config.viewport = pass.viewport() ;

      data().createLayout() ;
      data().createPipeline() ;
    }

    void Pipeline::setPushConstanceByteSize( unsigned size )
    {
      data().push_constant_size = size ;
    }
    
    void Pipeline::setPushConstantStageFlag( unsigned stage )
    {
      data().push_constant_flags = static_cast<vk::ShaderStageFlags>( stage ) ;
    }
    
    void Pipeline::reset()
    {
      data().device.device().destroy( data().pipeline, nullptr ) ;
      data().device.device().destroy( data().layout  , nullptr ) ;
    }

    const vk::Pipeline& Pipeline::pipeline() const
    {
      return data().pipeline ;
    }

    const vk::PipelineLayout& Pipeline::layout() const
    {
      return data().layout ;
    }

    PipelineData& Pipeline::data()
    {
      return *this->pipeline_data ;
    }

    const PipelineData& Pipeline::data() const
    {
      return *this->pipeline_data ;
    }
  }
}
