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

#pragma once

struct cudaChannelFormatDesc  ;

namespace nyx
{
  
  /** Forward declared Memory object for friendship.
   */
  template<typename IMPL>
  class Memory ;
  
  template<typename IMPL, typename TYPE>
  class Array ;
  
  template<typename IMPL, typename TYPE>
  class Image ;

  namespace cg
  {
    class Cuda
    {
      public:
        using Device         = unsigned              ;
        using Buffer         = ::nyx::Memory<Cuda>   ;
        using ImageFormat    = cudaChannelFormatDesc ;
        using ImageLayout    = int                   ;
        using Memory         = void*                 ;
        using CommandRecord  = unsigned              ;
        using MemoryFlags    = unsigned              ;
        
        /** Static method to initialize this implementation.
         */
        static void initialize() ;
        
        /** Static method to convert a cuda error to an error of the library.
         * @param error An error defined by cuda.
         * @return An error defined by the library.
         */
        static unsigned convertError( unsigned error ) ;

      private:
        
        
        /** Typedef to avoid using void* directly.
         */
        typedef void* Data ;
        
        /** Friend declaration of memory object to use this implementation for.
         */
        friend class ::nyx::Memory<Cuda> ;
  
        /** Default constructor.
         */
        Cuda() = default ;
  
        /** Default Deconstructor.
         */
        ~Cuda() = default ;
  
        /** Method to copy data from the host ( RAM ) to the GPU ( VRAM ).
         * @param src The source data on the host.
         * @param dst The memory handle to write to on the GPU
         * @param gpu The device to use for this operation.
         * @param amt The amount of data to copy.
         */
        void copyToDevice( const void* src, Memory& dst, Cuda::Device& gpu, unsigned amt, unsigned src_offset = 0, unsigned dst_offset = 0 ) ;
  
        /** Method to copy data from the GPU ( VRAM ) to the host ( RAM ).
         * @param src The source memory handle on the GPU.
         * @param dst The pointer to write to on the host.
         * @param gpu The device to use for this operation.
         * @param amt The amount of data to copy.
         */
        void copyToHost( const void* src, Data dst, Cuda::Device& gpu, unsigned amt, unsigned src_offset = 0, unsigned dst_offset = 0  ) ;
  
        /** Method to release the input memory handle.
         * @param mem The memory object to release.
         * @param gpu The device the memory object was allocated on.
         */
        void free( Memory& mem, Cuda::Device& gpu ) ;
  
        /** Method to create & allocate memory on the GPU.
         * @param size The size of the memory to allocate.
         * @param gpu The GPU to allocate data on.
         * @return Allocated memory on the GPU.
         */
        Memory createMemory( const Cuda::Device& gpu, unsigned size ) ;
        
        /** Method to create & allocate memory on the GPU.
         * @param size The size of the memory to allocate.
         * @param gpu The GPU to allocate data on.
         * @param mem_flags The memory property flags to use for creation of this memory object.
         * @return Allocated memory on the GPU.
         */
        Memory createMemory( const Cuda::Device& gpu, unsigned size, Cuda::MemoryFlags mem_flags ) ;
    };
  }
  
  /** Helper Aliases for useful types.
   */
  template<typename TYPE>
  using CudaImage = nyx::Image<nyx::cg::Cuda, TYPE> ;
  
  template<typename TYPE>
  using CudaArray  = nyx::Array <nyx::cg::Cuda, TYPE> ;
  
  using CudaMemory     = nyx::Memory<nyx::cg::Cuda              > ;
  using FloatCudaImage = nyx::Image<nyx::cg::Cuda, float        > ;
  using CharCudaImage  = nyx::Image<nyx::cg::Cuda, unsigned char> ;
}

