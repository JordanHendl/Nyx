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
 * File:   CommandBuffer.h
 * Author: Jordan Hendl
 *
 * Created on December 30, 2020, 1:42 PM
 */
#pragma once 

namespace vk
{
  class CommandBuffer ;
  class Fence         ;
}

namespace nyx
{
  template<typename Impl, typename Type>
  class Array ;
  
  enum class PipelineStage ;
  namespace vkg
  {
    /** Forward declared library objects.
     */
    class Queue      ;
    class Device     ;
    class Buffer     ;
    class RenderPass ;
    class Pipeline   ;
    class Vulkan     ;
    class Descriptor ;

    /** Class for handling command buffer generation & management.
     */
    class CommandBuffer
    {
      public:
        
        /** The level of command buffer this object is.
         */
        enum class Level
        {
          Primary,
          Secondary
        };

        /** Default constructor.
         */
        CommandBuffer() ;
        
        /** Copy constructor. Assigns this object to the input.
         * @param cmd The command buffer to assign this object to.
         */
        CommandBuffer( const CommandBuffer& cmd ) ;
        
        /** Default deconstructor.
         */
        ~CommandBuffer() ;
        
        /** Method to push a value onto the command buffer as a push constant.
         * @param value The value to push.
         * @param stage The stage of the pipeline that the constant is used in.
         */
        template<typename Type>
        void pushConstant( const Type& value, nyx::PipelineStage stage ) ;
        
        /** Method to bind a descriptor to this command buffer.
         * @note Requires a pipeline already bound to this object.
         * @param descriptor The descriptor to bind.
         */
        void bind( const nyx::vkg::Descriptor& descriptor ) ;

        /** Method to bind a pipeline to this command buffer.
         * @param pipeline The pipeline to bind.
         */
        void bind( const nyx::vkg::Pipeline& pipeline ) ;
        
        /** Assignment operator. Assigns this object to the input.
         * @param cmd The command buffer to assign this object to.
         * @return A reference to this object after assignment.
         */
        CommandBuffer& operator=( const CommandBuffer& cmd ) ;
        
        /** Method to initialize this command buffer object.
         * @param queue The queue to use for this object's creation.
         * @param count The number of command buffers to generate.
         * @param level The command buffer level of this object.
         */
        void initialize( const nyx::vkg::Queue& queue, unsigned count, CommandBuffer::Level level = CommandBuffer::Level::Primary ) ;
        
        /** Method to combine a secondary command buffer into this object.
         * @param cmd The secondary command buffer to consume into this object.
         */
        void combine( const CommandBuffer& cmd ) ;
        
        /** Method to retrieve the number of command buffers generated by this object.
         * @return The amount of command buffers generated by this object.
         */
        unsigned size() const ;
        
        /** Method to retrieve the queue associated with this command buffer & command pool.
         * @return The queue associated with this command buffer & command pool.
         */
        const nyx::vkg::Queue& queue() const ;
        
        /** Method to retrive the library device associated with this object.
         * @return The library device associated with this object.
         */
        unsigned device() const ;
        
        /** Method to retrieve the specified command buffer at the index.
         * @param idx The index of command buffer to retrieve.
         * @return The command buffer at the specified index.
         */
        const vk::CommandBuffer& buffer( unsigned idx = 0 ) const ;
        
        /** Method to retrieve the level of command buffer this object is.
         * @return The level ( Primary or Secondary ) of this object.
         */
        Level level() const ;

        /** Method to retrieve a pointer to the start of this object's list of vulkan command buffers.
         * @return A const pointer to the start of this object's command buffer list.
         */
        const vk::CommandBuffer* pointer() const ;

        /** Method to record a draw operation on this object's command buffers.
         * @param buffer The buffer containing vertex data to draw.
         * @param offset The elementoffset into the buffer to start drawing at.
         */
        template<typename Type>
        void draw( const nyx::Array<vkg::Vulkan, Type>& array, unsigned offset = 0 ) ;
        
//        /** Method to record a instanced draw operation on this object's command buffers.
//         * @param buffer The buffer containing vertex data to draw instanced.
//         * @param instance_count The amount of times to instance draw the buffer.
//         * @param offset The offset into the buffer to start drawing.
//         * @param first The first index into the buffer to draw at.
//         */
//        void drawInstanced( const nyx::vkg::Buffer& buffer, unsigned instance_count, unsigned offset = 0, unsigned first = 0 ) ;
//        
        /** Method to record an indexed draw command to this object's command buffers.
         * @param indices The buffer containing the indices of the vertex buffer to draw.
         * @param vertices The vertex buffer used for drawing.
         */
        template<typename Type, typename Type2>
        void drawIndexed(const nyx::Array<vkg::Vulkan, Type2>& indices, const nyx::Array<vkg::Vulkan, Type>& vertices ) ;
        
        /** Method to begin all this object's command buffers record using input render pass.
         * @param render_pass Method to begin the render pass & all this object's command buffers record as well.
         * @param index The index of command buffer to start recording.
         */
        void record( const nyx::vkg::RenderPass& render_pass, unsigned index ) ;
        
        /** Method to begin all of this object's command buffers recording.
         */
        void record( unsigned index ) ;
        
        /** Method to begin all this object's command buffers record using input render pass.
         * @param render_pass Method to begin the render pass & all this object's command buffers record as well.
         */
        void record( const nyx::vkg::RenderPass& render_pass ) ;
        
        /** Method to begin all of this object's command buffers recording.
         */
        void record() ;
        
        /** Method to stop all recording of this object's command buffers. If started a render pass, stops as well.
         * @param index The index of command buffer to stop.
         */
        void stop( unsigned index ) ;

        /** Method to stop all recording of this object's command buffers. If started a render pass, stops as well.
         */
        void stop() ;
        
        
        /** Method to release all of this object's allocated data.
         */
        void reset() ;
        
      private:
        
        /** Base method to use a buffer as vertices to draw.
         * @param buffer The buffer to use for vertices.
         * @param count The amount of vertices in the buffer.
         * @param offset The offset of the buffer to start at.
         */
        void drawBase( const nyx::vkg::Buffer& buffer, unsigned count, unsigned offset = 0 ) ;\

        /** Base method to use a buffer as vertices to draw.
         * @param index The buffer to use for indices.
         * @param vert The buffer to use for vertices.
         * @param index_count The amount of indices in the buffer.
         * @param vert_count The amount of vertices in the buffer.
         * @param offset The offset of the buffer to start at.
         */
        void drawIndexedBase( const nyx::vkg::Buffer& index, const nyx::vkg::Buffer& vert, unsigned index_count, unsigned vert_count, unsigned offset = 0 ) ;
        
        /** Private method for pushing a value as a push-constant to this command buffer.
         * @param value The pointer value to push onto the Device.
         * @param byte_size The size in bytes of the object being pushed.
         * @param stage_flags The stage that the push constant is used on.
         */
        void pushConstantBase( const void* value, unsigned byte_size, nyx::PipelineStage stage_flags ) ;

        /** Forward declared structure containing this object's data.
         */
        struct CommandBufferData *cmd_data ;
        
        /** Method to retrieve a reference to this object's internal data structure.
         * @return Reference to this object's internal data structure.
         */
        CommandBufferData& data() ;
        
        /** Method to retrieve a const-reference to this object's internal data structure.
         * @return Const-reference to this object's internal data structure.
         */
        const CommandBufferData& data() const ;
    };
    
    template<typename Type>
    void CommandBuffer::draw( const nyx::Array<vkg::Vulkan,Type>& array, unsigned offset )
    {
      this->drawBase( array, array.size(), offset ) ;
    }

    template<typename Type, typename Type2>
    void CommandBuffer::drawIndexed( const nyx::Array<vkg::Vulkan, Type2>& indices, const nyx::Array<vkg::Vulkan, Type>& vertices )
    {
      this->drawIndexedBase( indices, vertices, indices.size(), vertices.size(), 0 ) ;
    }

    template<typename Type>
    void CommandBuffer::pushConstant( const Type& value, nyx::PipelineStage stage )
    {
      this->pushConstantBase( static_cast<const void*>( &value ), sizeof( Type ), stage ) ;
    }
  }
}

