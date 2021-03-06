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

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_ASSERT_ON_RESULT
#define VULKAN_HPP_NOEXCEPT
#define VULKAN_HPP_NOEXCEPT_WHEN_NO_EXCEPTIONS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "Vulkan.h"
#include "Device.h"
#include "library/Image.h"
#include "library/Window.h"
#include "library/Chain.h"
#include <library/Renderer.h>
#include <library/Memory.h>
#include <library/RenderPass.h>
#include <algorithm>
#include <iostream>
#include <stdint.h>
#include <vector>
#include <string>
#include <map>
#include <vulkan/vulkan.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

unsigned operator|( unsigned first, vk::MemoryPropertyFlagBits second )
{
  return first | static_cast<unsigned>( second ) ;
}

extern bool SDL2_INITIALIZED ;

 namespace nyx
 {
   namespace vkg
   {
     
    #if defined ( __unix__ ) || defined( _WIN32 )
      constexpr const char* END_COLOR    = "\x1B[m"     ;
      constexpr const char* COLOR_RED    = "\u001b[31m" ;
      constexpr const char* COLOR_GREEN  = "\u001b[32m" ;
      constexpr const char* COLOR_YELLOW = "\u001b[33m" ;
      constexpr const char* COLOR_GREY   = "\x1B[1;30m" ;
      constexpr const char* UNDERLINE    = "\u001b[4m"  ;
    #else
      constexpr const char* END_COLOR    = "" ;
      constexpr const char* COLOR_GREEN  = "" ;
      constexpr const char* COLOR_YELLOW = "" ;
      constexpr const char* COLOR_GREY   = "" ;
      constexpr const char* COLOR_RED    = "" ;
      constexpr const char* COLOR_WHITE  = "" ;
    #endif

    /** Static function for getting a color code from a vulkan severity.
     * @param severity The severity to determing the color of.
     * @return The color code of the severity.
     */
    static const char* colorFromSeverity( Vulkan::Severity severity ) ;

    /** Static function to find the appropriate memory given the input flags and filter.
     * @param filter The mask to filter results.
     * @param flag The memory flag to look for.
     * @param device The device to look for memory on.
     * @return The mask of memory type found.
     */
    static uint32_t memType( uint32_t filter, ::vk::MemoryPropertyFlags flag, ::vk::PhysicalDevice device ) ;
     
    /** Static function to act as a default memory handler.
     * @param error The error to handle.
     */
    static void defaultHandler( nyx::vkg::Vulkan::Error error ) ;

    /** The structure to contain all of the global vkg library data.
     */
    struct VulkanData
    {
      typedef void ( *Callback )( Vulkan::Error ) ;
      
      using WindowMap = std::map<unsigned, nyx::Window<Vulkan>*> ;
      
      Callback                 error_cb          ;
      WindowMap                windows           ;
      Vulkan::ErrorHandler*    handler           ;
      vkg::Instance            instance          ;
      std::vector<vkg::Device> devices           ;
      std::vector<std::string> validation_layers ;
      std::vector<std::string> device_ext        ;
      
      /** Default constructor.
       */
      VulkanData() ;
    };
     
    /** The container for a vulkan surface's data.
     */
    struct SurfaceData
    {
      vk::SurfaceKHR surface ;
    };

    /** Static container for all of the vulkan library's global data.
     */
    static VulkanData data ;
    
    const char* colorFromSeverity( Vulkan::Severity severity )
    {
      switch ( severity )
      {
        case Vulkan::Severity::Info    : return vkg::COLOR_GREY   ;
        case Vulkan::Severity::Warning : return vkg::COLOR_YELLOW ;
        case Vulkan::Severity::Fatal   : return vkg::COLOR_RED    ;
        default : return vkg::COLOR_RED ;
      }
    }
    
    uint32_t memType( uint32_t filter, ::vk::MemoryPropertyFlags flag, ::vk::PhysicalDevice device )
    {
      ::vk::PhysicalDeviceMemoryProperties mem_prop ;

      mem_prop = device.getMemoryProperties() ;
      for( unsigned index = 0; index < mem_prop.memoryTypeCount; index++ )
      {
        if( filter & ( 1 << index ) && ( mem_prop.memoryTypes[ index ].propertyFlags & flag ) == flag )
        {
          return index ;
        }
      }
      return 0 ;
    }

    void defaultHandler( nyx::vkg::Vulkan::Error error )
    {
      auto severity = error.severity() ;

      std::cout << colorFromSeverity( severity ) << "-- " << severity.toString() << " | " << "Nyx::vkg Error: " << error.toString() << "" << vkg::END_COLOR << std::endl ;
      if( severity == Vulkan::Severity::Fatal ) exit( -1 ) ;
    }

    VulkanData::VulkanData()
    {
      this->error_cb = &vkg::defaultHandler ;
      this->handler  = nullptr ;
    }

    Memory::Memory()
    {
      this->val = 0x0 ;
    }

    Memory::~Memory()
    {
      this->val = 0x0 ;
    }

    Memory::Memory( vk::DeviceMemory& val )
    {
      *this = val ;
    }

    Memory& Memory::operator=( vk::DeviceMemory& val )
    {
      static_assert( sizeof( vk::DeviceMemory ) == sizeof( void* ), "Vulkan Device Memory cannot be wrapped up." ) ;
      this->val = reinterpret_cast<void*>( static_cast<VkDeviceMemory>( val ) ) ;
      
      return *this ;
    }
    
    Memory& Memory::operator=( void* val )
    {
      static_assert( sizeof( vk::DeviceMemory ) == sizeof( void* ), "Vulkan Device Memory cannot be wrapped up." ) ;
      this->val = val ;
      
      return *this ;
    }

    Memory::operator bool() const
    {
      return reinterpret_cast<VkDeviceMemory>( this->val ) ;
    }

    Memory::operator vk::DeviceMemory()
    {
      return static_cast<vk::DeviceMemory>( reinterpret_cast<VkDeviceMemory>( this->val ) ) ;
    }

    Memory::operator vk::DeviceMemory() const
    {
      return static_cast<vk::DeviceMemory>( reinterpret_cast<VkDeviceMemory>( this->val ) ) ;
    }

    Surface::Surface()
    {
      this->surface_data = new SurfaceData() ;
    }
    
    Surface::Surface( const Surface& surface )
    {
      this->surface_data = new SurfaceData() ;
      *this = surface ;
    }
    
    Surface::~Surface()
    {
      delete this->surface_data ;
    }
    
    Surface& Surface::operator=( const Surface& surface )
    {
      *this->surface_data = *surface.surface_data ;
      return *this ;
    }
    
    const vk::SurfaceKHR& Surface::surface() const
    {
      return data().surface ;
    }
    
    SurfaceData& Surface::data()
    {
      return *this->surface_data ;
    }
    
    const SurfaceData& Surface::data() const
    {
      return *this->surface_data ;
    }
    
    vkg::Vulkan::Severity::Severity()
    {
      this->sev = Severity::None ;
    }
    
    vkg::Vulkan::Severity::Severity( const vkg::Vulkan::Severity& severity )
    {
      this->sev = severity.sev ;
    }
    
    vkg::Vulkan::Severity::Severity( unsigned error )
    {
      this->sev = error ;
    }
    
    unsigned vkg::Vulkan::Severity::severity() const
    {
      return *this ;
    }
    
    vkg::Vulkan::Severity::operator unsigned() const
    {
      return this->sev ;
    }
    
    const char* vkg::Vulkan::Severity::toString() const
    {
      switch( this->sev )
      {
        case Severity::Warning : return "Warning" ;
        case Severity::Fatal   : return "Fatal"   ;
        case Severity::None    : return "None"    ;
        case Severity::Info    : return "Info"    ;
        default : return "Unknown Severity" ;
      }
    }
    
    vkg::Vulkan::Error::Error()
    {
      this->err = Error::None ;
    }
    
    vkg::Vulkan::Error::Error( const vkg::Vulkan::Error& error )
    {
      this->err = error.err ;
    }
    
    vkg::Vulkan::Error::Error( unsigned error )
    {
      this->err = error ;
    }
    
    unsigned vkg::Vulkan::Error::error() const
    {
      return *this ;
    }
    
    vkg::Vulkan::Error::operator unsigned() const
    {
      return this->err ;
    }
    
    const char* vkg::Vulkan::Error::toString() const
    {
      switch( this->err )
      {
        case Error::DeviceLost           : return "DeviceLost : The device has been lost."                                                 ;
        case Error::DeviceNotFound       : return "DeviceNotFound: Device requested was not found on the system"                           ;
        case Error::FeatureNotPresent    : return "FeatureNotPresent: A Requested Feature is not supported by this system"                 ;
        case Error::SuboptimalKHR        : return "SuboptimalKHR: The VKG surface is not compatible with the window"                       ;
        case Error::OutOfDataKHR         : return "OutOfDataKHR: The VKG swapchain is not capable of presenting to the specified surface." ;
        case Error::InitializationFailed : return "InitializationFailed: Vulkan initialization failed!"                                    ;
        case Error::OutOfDeviceMemory    : return "Out of device memory: Device memory available has been depleted."                       ;
        case Error::MemoryMapFailed      : return "Memory Map Failure: A Host-GPU memory mapping has failed."                              ;
        case Error::ValidationFailed     : return "Validation Layer Failed."                                                               ;
        case Error::NativeWindowInUse    : return "A Native window is already in use."                                                     ;
        default : return "Unknown Error" ;
      }
    }
    
    Vulkan::Severity vkg::Vulkan::Error::severity() const
    {
      switch( this->err )
      {
        case Error::DeviceNotFound       : return Severity::Warning ;
        case Error::FeatureNotPresent    : return Severity::Warning ;
        case Error::SuboptimalKHR        : return Severity::Warning ;
        case Error::NativeWindowInUse    : return Severity::Fatal   ;
        case Error::ValidationFailed     : return Severity::Fatal   ;
        case Error::DeviceLost           : return Severity::Fatal   ;
        case Error::OutOfDataKHR         : return Severity::Fatal   ;
        case Error::InitializationFailed : return Severity::Fatal   ;
        case Error::OutOfDeviceMemory    : return Severity::Fatal   ;
        default : return Severity::Fatal ;
      }
    }

    void Vulkan::add( nyx::vkg::Vulkan::Error error )
    {
      if( error != Vulkan::Error::Success )
      {
        if( data.error_cb != nullptr )
        {
          ( data.error_cb )( error ) ;
        }
        
        if( data.handler != nullptr )
        {
          data.handler->handleError( error ) ;
        }
      }
    }
    
    void Vulkan::add( vk::Result error )
    {
      Vulkan::add( Vulkan::convert( error ) ) ;
    }
     
    void Vulkan::setApplicationName( const char* application_name )
    {
      data.instance.setApplicationName( application_name ) ;
    }

    void Vulkan::addInstanceExtension( const char* extension )
    {
      data.instance.addExtension( extension ) ;
    }

    void Vulkan::addDeviceExtension( const char* extension, unsigned idx )
    {
      idx = idx ; // TODO handle this.
      data.device_ext.push_back( extension ) ;
    }

    void Vulkan::addValidationLayer( const char* layer_name )
    {
      data.validation_layers.push_back( layer_name ) ;
    }

    void Vulkan::setErrorHandler( void ( *error_handler )( nyx::vkg::Vulkan::Error ) )
    {
      data.error_cb = error_handler ;
    }
    
    const vkg::Device& Vulkan::device( unsigned id )
    {
      static vkg::Device dummy ;
      if( id < data.devices.size() ) return data.devices[ id ] ;
      else
      {
        Vulkan::add( Vulkan::Error::DeviceNotFound ) ;
        if( !data.devices.empty() ) return data.devices[ 0 ] ;
        return dummy ;
      }
    }

    void Vulkan::deviceSynchronize( unsigned gpu )
    {
      Vulkan::device( gpu ).wait() ;
    }
    
    void Vulkan::setErrorHandler( nyx::vkg::Vulkan::ErrorHandler* handler )
    {
      data.handler = handler ;
    }

    vkg::Vulkan::Error Vulkan::convert( vk::Result error )
    {
      switch( error )
      {
        case vk::Result::eErrorDeviceLost           : return Vulkan::Error::DeviceLost           ;
        case vk::Result::eSuccess                   : return Vulkan::Error::Success              ;
        case vk::Result::eErrorFeatureNotPresent    : return Vulkan::Error::FeatureNotPresent    ;
        case vk::Result::eErrorInitializationFailed : return Vulkan::Error::InitializationFailed ;
        case vk::Result::eErrorOutOfDateKHR         : return Vulkan::Error::RecreateSwapchain    ;
        case vk::Result::eSuboptimalKHR             : return Vulkan::Error::RecreateSwapchain    ;
        case vk::Result::eErrorOutOfDeviceMemory    : return Vulkan::Error::OutOfDeviceMemory    ;
        case vk::Result::eErrorMemoryMapFailed      : return Vulkan::Error::MemoryMapFailed      ;
        case vk::Result::eErrorValidationFailedEXT  : return Vulkan::Error::ValidationFailed     ;
        case vk::Result::eErrorNativeWindowInUseKHR : return Vulkan::Error::NativeWindowInUse    ;
        default : return Vulkan::Error::Unknown ;
      }
    }

    vk::ShaderStageFlags Vulkan::convert( nyx::PipelineStage stage )
    {
      switch( stage )
      {
        case nyx::PipelineStage::Vertex   : return vk::ShaderStageFlagBits::eVertex              ;
        case nyx::PipelineStage::Fragment : return vk::ShaderStageFlagBits::eFragment            ;
        case nyx::PipelineStage::Compute  : return vk::ShaderStageFlagBits::eCompute             ;
        case nyx::PipelineStage::TessC    : return vk::ShaderStageFlagBits::eTessellationControl ;
        default : return vk::ShaderStageFlagBits::eVertex ;
      }
    }
    
    vk::PipelineStageFlags Vulkan::convert( nyx::GPUStages stage )
    {
      switch( stage )
      {
         case nyx::GPUStages::Top                              : return vk::PipelineStageFlagBits::eTopOfPipe                        ;                           
         case nyx::GPUStages::DrawIndirect                     : return vk::PipelineStageFlagBits::eDrawIndirect                     ;                              
         case nyx::GPUStages::VertexInput                      : return vk::PipelineStageFlagBits::eVertexInput                      ;                             
         case nyx::GPUStages::VertexShader                     : return vk::PipelineStageFlagBits::eVertexShader                     ;                              
         case nyx::GPUStages::TessellationControlShader        : return vk::PipelineStageFlagBits::eTessellationControlShader        ;                                           
         case nyx::GPUStages::TessellationEvaluationShader     : return vk::PipelineStageFlagBits::eTessellationEvaluationShader     ;                                              
         case nyx::GPUStages::GeometryShader                   : return vk::PipelineStageFlagBits::eGeometryShader                   ;                                
         case nyx::GPUStages::FragmentShader                   : return vk::PipelineStageFlagBits::eFragmentShader                   ;                                
         case nyx::GPUStages::EarlyFragmentTests               : return vk::PipelineStageFlagBits::eEarlyFragmentTests               ;                                                                                   
         case nyx::GPUStages::LateFragmentTests                : return vk::PipelineStageFlagBits::eLateFragmentTests                ;                                   
         case nyx::GPUStages::ColorAttachmentOutput            : return vk::PipelineStageFlagBits::eColorAttachmentOutput            ;                                       
         case nyx::GPUStages::ComputeShader                    : return vk::PipelineStageFlagBits::eComputeShader                    ;                               
         case nyx::GPUStages::Transfer                         : return vk::PipelineStageFlagBits::eTransfer                         ;                          
         case nyx::GPUStages::BottomOfPipe                     : return vk::PipelineStageFlagBits::eBottomOfPipe                     ;                              
         case nyx::GPUStages::Host                             : return vk::PipelineStageFlagBits::eHost                             ;                                                                     
         case nyx::GPUStages::AllGraphics                      : return vk::PipelineStageFlagBits::eAllGraphics                      ;                             
         case nyx::GPUStages::AllCommands                      : return vk::PipelineStageFlagBits::eAllCommands                      ;                             
         case nyx::GPUStages::ShadingRateImage                 : return vk::PipelineStageFlagBits::eShadingRateImageNV               ;                                                                                                                                  
         case nyx::GPUStages::TaskShader                       : return vk::PipelineStageFlagBits::eTaskShaderNV                     ;                              
         case nyx::GPUStages::MeshShader                       : return vk::PipelineStageFlagBits::eMeshShaderNV                     ;                              
         case nyx::GPUStages::CommandPreprocess                : return vk::PipelineStageFlagBits::eCommandPreprocessNV              ;                                     
         case nyx::GPUStages::AccelerationStructureBuild       : return vk::PipelineStageFlagBits::eAccelerationStructureBuildNV     ;                                              
         case nyx::GPUStages::RayTracing                       : return vk::PipelineStageFlagBits::eRayTracingShaderNV               ;                                
         default : return vk::PipelineStageFlagBits::eAllCommands ;
      }
    }
    
    vk::AttachmentDescription Vulkan::convert( const nyx::Attachment& attachment )
    {
      using StoreOps = vk::AttachmentStoreOp ;
      using LoadOps  = vk::AttachmentLoadOp  ;
      
      const auto format = Vulkan::convert( attachment.format() )                                                                                  ;
      const auto layout = Vulkan::convert( attachment.layout() )                                                                                  ;
      const auto stencil_store = attachment.storeStencil() ? StoreOps::eStore : StoreOps::eDontCare                                               ;
      const auto stencil_load  = attachment.testStencil()  ? LoadOps::eLoad   : attachment.clearStencil() ? LoadOps::eClear : LoadOps::eDontCare  ;
      const auto load_op       = LoadOps::eClear  ; /// JH TODO Change this.
      const auto store_op      = StoreOps::eStore ;

      vk::AttachmentDescription desc ;
      
      desc.setSamples       ( vk::SampleCountFlagBits::e1 ) ;
      desc.setLoadOp        ( load_op                     ) ;
      desc.setStoreOp       ( store_op                    ) ;
      desc.setFormat        ( format                      ) ;
      desc.setInitialLayout ( vk::ImageLayout::eUndefined ) ;
      desc.setStencilLoadOp ( stencil_load                ) ;
      desc.setStencilStoreOp( stencil_store               ) ;
      desc.setFinalLayout   ( layout                      ) ;
      
      return desc ;
    }
    vk::Format Vulkan::convert( nyx::ImageFormat format )
    {
      switch( format )
      {
        case nyx::ImageFormat::R8      : return vk::Format::eR8Srgb             ;
        case nyx::ImageFormat::RGB8    : return vk::Format::eR8G8B8Srgb         ;
        case nyx::ImageFormat::BGR8    : return vk::Format::eB8G8R8Srgb         ;
        case nyx::ImageFormat::RGBA8   : return vk::Format::eR8G8B8A8Srgb       ;
        case nyx::ImageFormat::BGRA8   : return vk::Format::eB8G8R8A8Srgb       ;
        case nyx::ImageFormat::R32I    : return vk::Format::eR32Sint            ;
        case nyx::ImageFormat::RGB32I  : return vk::Format::eR32G32B32Sint      ;
        case nyx::ImageFormat::RGBA32I : return vk::Format::eR32G32B32A32Sint   ;
        case nyx::ImageFormat::R32F    : return vk::Format::eR32Sfloat          ;
        case nyx::ImageFormat::RGB32F  : return vk::Format::eR32G32B32Sfloat    ;
        case nyx::ImageFormat::RGBA32F : return vk::Format::eR32G32B32A32Sfloat ;
        case nyx::ImageFormat::D32F    : return vk::Format::eD24UnormS8Uint     ;
        default : return vk::Format::eUndefined ;
      }
    }

    nyx::ImageFormat Vulkan::convert( vk::Format format )
    {
      switch( format )
      {
        case vk::Format::eR8Srgb             : return nyx::ImageFormat::R8      ;
        case vk::Format::eR8G8B8Srgb         : return nyx::ImageFormat::RGB8    ;
        case vk::Format::eB8G8R8Srgb         : return nyx::ImageFormat::BGR8    ;
        case vk::Format::eR8G8B8A8Srgb       : return nyx::ImageFormat::RGBA8   ;
        case vk::Format::eB8G8R8A8Srgb       : return nyx::ImageFormat::BGRA8   ;
        case vk::Format::eR32Sint            : return nyx::ImageFormat::R32I    ;
        case vk::Format::eR32G32B32Sint      : return nyx::ImageFormat::RGB32I  ;
        case vk::Format::eR32G32B32A32Sint   : return nyx::ImageFormat::RGBA32I ;
        case vk::Format::eR32Sfloat          : return nyx::ImageFormat::R32F    ;
        case vk::Format::eR32G32B32Sfloat    : return nyx::ImageFormat::RGB32F  ;
        case vk::Format::eR32G32B32A32Sfloat : return nyx::ImageFormat::RGBA32F ;
        case vk::Format::eD32Sfloat          : return nyx::ImageFormat::D32F    ;
        case vk::Format::eD24UnormS8Uint     : return nyx::ImageFormat::D32F    ;
        default : return nyx::ImageFormat::RGB8 ;
      }
    }

    nyx::ImageLayout Vulkan::convert( vk::ImageLayout layout )
    {
      switch( layout )
      {
        case vk::ImageLayout::eUndefined               : return nyx::ImageLayout::Undefined       ;
        case vk::ImageLayout::eGeneral                 : return nyx::ImageLayout::General         ;
        case vk::ImageLayout::eColorAttachmentOptimal  : return nyx::ImageLayout::ColorAttachment ;
        case vk::ImageLayout::eShaderReadOnlyOptimal   : return nyx::ImageLayout::ShaderRead      ;
        case vk::ImageLayout::eTransferSrcOptimal      : return nyx::ImageLayout::TransferSrc     ;
        case vk::ImageLayout::eTransferDstOptimal      : return nyx::ImageLayout::TransferDst     ;
        case vk::ImageLayout::ePresentSrcKHR           : return nyx::ImageLayout::PresentSrc      ;
        case vk::ImageLayout::eDepthReadOnlyOptimalKHR : return nyx::ImageLayout::DepthRead       ;
        
        default : return nyx::ImageLayout::Undefined ;
      };
    }
    
    vk::ImageLayout Vulkan::convert( nyx::ImageLayout layout )
    {
      switch( layout )
      {
        case nyx::ImageLayout::Undefined       : return vk::ImageLayout::eUndefined                     ;
        case nyx::ImageLayout::General         : return vk::ImageLayout::eGeneral                       ;
        case nyx::ImageLayout::ColorAttachment : return vk::ImageLayout::eColorAttachmentOptimal        ;
        case nyx::ImageLayout::ShaderRead      : return vk::ImageLayout::eShaderReadOnlyOptimal         ;
        case nyx::ImageLayout::TransferSrc     : return vk::ImageLayout::eTransferSrcOptimal            ;
        case nyx::ImageLayout::TransferDst     : return vk::ImageLayout::eTransferDstOptimal            ;
        case nyx::ImageLayout::PresentSrc      : return vk::ImageLayout::ePresentSrcKHR                 ;
        case nyx::ImageLayout::DepthRead       : return vk::ImageLayout::eDepthReadOnlyOptimalKHR       ;
        case nyx::ImageLayout::DepthStencil    : return vk::ImageLayout::eDepthStencilAttachmentOptimal ;
        default : return vk::ImageLayout::eUndefined ;
      };
    }

    nyx::ImageUsage Vulkan::convert( vk::ImageUsageFlagBits usage )
    {
      switch( static_cast<vk::ImageUsageFlagBits>( usage ) )
      {
        case vk::ImageUsageFlagBits::eTransferSrc            : return nyx::ImageUsage::TransferSrc          ;
        case vk::ImageUsageFlagBits::eTransferDst            : return nyx::ImageUsage::TransferDst          ;
        case vk::ImageUsageFlagBits::eSampled                : return nyx::ImageUsage::Sampled              ;
        case vk::ImageUsageFlagBits::eStorage                : return nyx::ImageUsage::Storage              ;
        case vk::ImageUsageFlagBits::eColorAttachment        : return nyx::ImageUsage::ColorAttachment      ;
        case vk::ImageUsageFlagBits::eDepthStencilAttachment : return nyx::ImageUsage::DepthStencil         ;
        case vk::ImageUsageFlagBits::eInputAttachment        : return nyx::ImageUsage::Input                ;
        case vk::ImageUsageFlagBits::eShadingRateImageNV     : return nyx::ImageUsage::ShadingRate          ;
        case vk::ImageUsageFlagBits::eFragmentDensityMapEXT  : return nyx::ImageUsage::VKEXTFragmentDensity ;
        default: return nyx::ImageUsage::Input ;
      };
    }
    
    vk::ImageUsageFlagBits Vulkan::convert( nyx::ImageUsage usage )
    {
      switch( usage )
      {
        case nyx::ImageUsage::TransferSrc          : return vk::ImageUsageFlagBits::eTransferSrc            ;
        case nyx::ImageUsage::TransferDst          : return vk::ImageUsageFlagBits::eTransferDst            ;
        case nyx::ImageUsage::Sampled              : return vk::ImageUsageFlagBits::eSampled                ;
        case nyx::ImageUsage::Storage              : return vk::ImageUsageFlagBits::eStorage                ;
        case nyx::ImageUsage::ColorAttachment      : return vk::ImageUsageFlagBits::eColorAttachment        ;
        case nyx::ImageUsage::DepthStencil         : return vk::ImageUsageFlagBits::eDepthStencilAttachment ;
        case nyx::ImageUsage::Input                : return vk::ImageUsageFlagBits::eInputAttachment        ;
        case nyx::ImageUsage::ShadingRate          : return vk::ImageUsageFlagBits::eShadingRateImageNV     ;
        case nyx::ImageUsage::VKEXTFragmentDensity : return vk::ImageUsageFlagBits::eFragmentDensityMapEXT  ;
        default: return vk::ImageUsageFlagBits::eInputAttachment ;
      };
    }
    
    nyx::ImageType Vulkan::convert( vk::ImageType type )
    {
      switch( type )
      {
        case vk::ImageType::e1D : return nyx::ImageType::n1D ;
        case vk::ImageType::e2D : return nyx::ImageType::n2D ;
        case vk::ImageType::e3D : return nyx::ImageType::n3D ;
        default : return nyx::ImageType::n2D ;
      };
    }
    
    vk::ImageType Vulkan::convert( nyx::ImageType type )
    {
      switch( type )
      {
        case nyx::ImageType::n1D : return vk::ImageType::e1D ;
        case nyx::ImageType::n2D : return vk::ImageType::e2D ;
        case nyx::ImageType::n3D : return vk::ImageType::e3D ;
        default : return vk::ImageType::e2D ;
      }; 
    }
    
    bool Vulkan::initialized()
    {
      return data.instance.initialized() && data.devices.size() != 0 ;
    }

    void Vulkan::initialize()
    {
      if( !Vulkan::initialized() )
      {
        for( const auto& str : data.validation_layers )
        {
          data.instance.addValidationLayer( str.c_str() ) ;
        }
        data.instance.initialize() ;

        data.devices.resize( data.instance.numDevices() ) ;
        
        for( unsigned index = 0; index < data.devices.size(); index++ )
        {
          for( const auto& str : data.validation_layers )
          {
            data.devices[ index ].addValidationLayer( str.c_str() ) ;
          }
          
          for( const auto& str : data.device_ext )
          {
            data.devices[ index ].addExtension( str.c_str() ) ;
          }
  
          data.devices[ index ].initialize( data.instance.device( index ), index ) ;
        }
      }
      
      data.device_ext       .clear() ;
      data.validation_layers.clear() ;
    }
      
    void Vulkan::copyToDevice( const void* src, Vulkan::Memory& dst, unsigned gpu, unsigned amt, unsigned src_offset, unsigned dst_offset )
    {
      Vulkan::initialize() ;
      const auto device = Vulkan::device( gpu ).device() ;

      ::vk::DeviceSize     offset ;
      ::vk::DeviceSize     amount ;
      ::vk::MemoryMapFlags flag   ;
      void*                mem    ;
      
      offset = dst_offset ;
      amount = amt        ;

      src    = static_cast<const void*>( reinterpret_cast<const unsigned char*>( src ) + src_offset ) ;

      Vulkan::add( device.mapMemory  ( dst, offset, amount, flag, &mem    ) ) ;
                   std::memcpy       ( mem, src, static_cast<size_t>( amt ) ) ;
                   device.unmapMemory( dst                                  ) ;
    }
    
    bool Vulkan::hasDevice( unsigned id )
    {
      if( id < data.devices.size() ) return data.devices[ id ].initialized() ;
      return false ;
    }

    vkg::Queue Vulkan::graphicsQueue( unsigned gpu )
    {
      Vulkan::initialize() ;
      return Vulkan::device( gpu ).graphicsQueue() ;
    }
    
    vkg::Queue Vulkan::computeQueue( unsigned gpu )
    {
      Vulkan::initialize() ;
      return Vulkan::device( gpu ).computeQueue() ;
    }
    
    vkg::Queue Vulkan::presentQueue( unsigned window_id, unsigned gpu )
    {
      static vkg::Queue dummy ;
      
      Vulkan::initialize() ;
      auto iter = vkg::data.windows.find( window_id ) ;
      
      if( iter != vkg::data.windows.end() )
      {
        return Vulkan::device( gpu ).presentQueue( iter->second->context() ) ;
      }
      
      return dummy ;
    }

    void Vulkan::copyToHost( const Vulkan::Memory& src, Vulkan::Data dst, unsigned gpu, unsigned amt, unsigned src_offset, unsigned dst_offset )
    {
      Vulkan::initialize() ;
      const auto device = Vulkan::device( gpu ).device() ;
      ::vk::DeviceSize     offset ;
      ::vk::DeviceSize     amount ;
      ::vk::MemoryMapFlags flag   ;
      void*                mem    ;
      
      offset = src_offset ;
      amount = amt        ;

      dst    = static_cast<void*>( reinterpret_cast<unsigned char*>( dst ) + dst_offset ) ;
      
      Vulkan::add( device.mapMemory  ( src, offset, amount, flag, &mem      ) ) ;
                   std::memcpy       ( dst, mem, static_cast<size_t>( amt )   ) ;
                   device.unmapMemory( src                                    ) ;
    }
    
    void Vulkan::free( Vulkan::Memory& mem, unsigned gpu )
    {
      Vulkan::initialize() ;
      if( mem )
      {
        const auto device = Vulkan::device( gpu ).device() ;
        if( device )
        {
          device.free ( mem ) ;
        }
      }
    }
    
    Vulkan::Memory Vulkan::createMemory( unsigned gpu, unsigned size, nyx::MemoryFlags flags, unsigned filter )
    {
      const auto                      device   = Vulkan::device( gpu ).device()                                                              ;
      const auto                      p_device = Vulkan::device( gpu ).physicalDevice()                                                      ;
      const ::vk::MemoryPropertyFlags flag     = static_cast<vk::MemoryPropertyFlags>( static_cast<VkMemoryPropertyFlags>( flags.value() ) ) ;
      Vulkan::Memory              mem       ;
      vk::MemoryAllocateInfo      info      ;
      vk::MemoryAllocateFlagsInfo flag_info ;

      Vulkan::initialize() ;
      
      flag_info.setFlags( vk::MemoryAllocateFlagBits::eDeviceAddress ) ;
      
      info.setAllocationSize ( size                              ) ;
      info.setMemoryTypeIndex( memType( filter, flag, p_device ) ) ;
      info.setPNext( &flag_info ) ;
      
      auto result = device.allocateMemory( info, nullptr ) ;
      
      Vulkan::add( result.result ) ;

      return result.value ;
    }
    
    Vulkan::Memory Vulkan::createMemory( unsigned gpu, unsigned size, unsigned filter )
    {
      nyx::MemoryFlags flags ;
      
      return this->createMemory( gpu, size, flags, filter ) ;
    }
    
    void Vulkan::handleWindowEvents( unsigned id )
    {
      auto iter = vkg::data.windows.find( id ) ;
      
      if( iter != vkg::data.windows.end() )
      {
        iter->second->handleEvents() ;
      }
    }

    void Vulkan::addWindow( unsigned id, const char* title, unsigned width, unsigned height )
    {
      nyx::Window<Vulkan>* window ;
      auto iter = vkg::data.windows.find( id ) ;
      
      if( iter == vkg::data.windows.end() )
      {
        window = new nyx::Window<Vulkan>() ;
        window->initialize( title, width, height ) ;
        vkg::data.windows.insert( iter, { id, window } ) ;
      }
    }
    
    bool Vulkan::hasWindow( unsigned id )
    {
      auto iter = vkg::data.windows.find( id ) ;
      if( iter == vkg::data.windows.end() )
      {
        return false ;
      }
      
      return true ;
    }
    
    void Vulkan::setWindowTitle( unsigned id, const char* title )
    {
      auto iter = vkg::data.windows.find( id ) ;
      
      if( iter != vkg::data.windows.end() )
      {
        iter->second->setTitle( title ) ;
      }
    }
    
    void Vulkan::setWindowResizable( unsigned id, bool value )
    {
      auto iter = vkg::data.windows.find( id ) ;
      
      if( iter != vkg::data.windows.end() )
      {
        iter->second->setResizable( value ) ;
      }
    }
    
    void Vulkan::setWindowMouseCapture( unsigned id, bool value )
    {
      auto iter = vkg::data.windows.find( id ) ;
      
      if( iter != vkg::data.windows.end() )
      {
        SDL_SetRelativeMouseMode( static_cast<SDL_bool>( value ) ) ;
        SDL_SetWindowGrab       ( iter->second->window().window(), static_cast<SDL_bool>( value ) ) ;
      }
    }
    
    void Vulkan::setWindowWidth( unsigned id, unsigned width )
    {
      auto iter = vkg::data.windows.find( id ) ;
      
      if( iter != vkg::data.windows.end() )
      {
        iter->second->setWidth( width ) ;
      }
    }
    
    void Vulkan::setWindowHeight( unsigned id, unsigned height )
    {
      auto iter = vkg::data.windows.find( id ) ;
      
      if( iter != vkg::data.windows.end() )
      {
        iter->second->setHeight( height ) ;
      }
    }
    
    void Vulkan::setWindowBorderless( unsigned id, bool value )
    {
      auto iter = vkg::data.windows.find( id ) ;
      
      if( iter != vkg::data.windows.end() )
      {
        iter->second->setBorderless( value ) ;
      }
    }
    
    vkg::Surface Vulkan::context( unsigned id )
    {
      static vkg::Surface dummy ;
      auto iter = vkg::data.windows.find( id ) ;
      
      if( iter != vkg::data.windows.end() )
      {
        return iter->second->context() ;
      }

      return dummy ;
    }

    
    const char* Vulkan::platformSurfaceInstanceExtensions()
    {
      unsigned                 amt ;
      std::vector<const char*> ext ;
      if( !SDL2_INITIALIZED )
      {
        SDL_Init( SDL_INIT_VIDEO ) ;
        SDL2_INITIALIZED = true ;
      }
      
      SDL_Window* dummy ;
      dummy = SDL_CreateWindow( "", 0, 0, 1280, 720, SDL_WINDOW_HIDDEN | SDL_WINDOW_VULKAN ) ;
      
      SDL_Vulkan_GetInstanceExtensions( dummy, &amt, nullptr ) ;
      ext.resize( amt ) ;
      SDL_Vulkan_GetInstanceExtensions( dummy, &amt, ext.data() ) ;
      
      SDL_DestroyWindow( dummy ) ;
      
      for( unsigned i = 0; i != ext.size(); i++ )
      {
        if( std::string( ext[ i ] ) != "VK_KHR_surface" ) return ext[ i ] ;
      }
      
      return ext[ 0 ] ;
    }

    Vulkan::Context Vulkan::contextFromBaseWindow( const nyx::sdl::Window& window )
    {
      vkg::Surface   out          ;
      VkSurfaceKHR   surface      ;
      VkInstance     instance     ;
      vk::SurfaceKHR vk_surface   ;
      
      Vulkan::initialize() ;
      
      instance = static_cast<VkInstance>( data.instance.instance() ) ;
      if( data.instance.initialized() )
      {
        if( !SDL_Vulkan_CreateSurface( window.window(), instance, &surface ) )
        {
          std::cout << "Error creating surface.\n" ; // TODO convert error.
          exit( -1 ) ;
        }

        vk_surface = surface ;
      }
      
      out.data().surface = vk_surface ;
      return out ;
     }
   }
 }
