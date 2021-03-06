#define VULKAN_HPP_ASSERT_ON_RESULT
#define VULKAN_HPP_NOEXCEPT
#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NOEXCEPT_WHEN_NO_EXCEPTIONS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_STORAGE_SHARED_EXPORT
#define VULKAN_HPP_STORAGE_SHARED

#include "Instance.h"
#include "Vulkan.h"
#include <vulkan/vulkan.hpp>
#include <string>
#include <vector>
#include <iostream>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
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

    /** Static variables for debug output.
     */
    static Instance::DebugOutputLevel output_level    = Instance::DebugOutputLevel::Normal ;
    static Instance::DebugSeverity    output_severity = Instance::DebugSeverity::WError    ;

    /** Method to convert a instance debug flag to vulkan flag.
     * @param level The flag to convert.
     * @return The Vulkan severity flag.
     */
    static vk::DebugUtilsMessageSeverityFlagsEXT toFlags( Instance::DebugSeverity level ) ;
    
    /** Method to convert a instance debug flag to vulkan flag.
     * @param level The flag to convert.
     * @return The Vulkan type flag.
     */
    static vk::DebugUtilsMessageTypeFlagsEXT toFlags( Instance::DebugOutputLevel type ) ;
    
    /** Static Vulkan debug callback for any vulkan instance errors. 
     * @param messageSeverity The severity of the debug message.
     * @param messageType The type of message.
     * @param pCallbackData The data containing the callback information.
     * @param pUserData The input user-specified data.
     * @return Whether or not the error was handled.
     */
    static  VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData )
    {
      const vk::DebugUtilsMessageSeverityFlagsEXT severity = static_cast<vk::DebugUtilsMessageSeverityFlagsEXT>( messageSeverity ) ;
      const vk::DebugUtilsMessageTypeFlagsEXT     type     = static_cast<vk::DebugUtilsMessageTypeFlagsEXT>    ( messageType     ) ;
      
      const char* COLOR ;
      if( severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose ) COLOR = COLOR_GREY   ;
      if( severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo    ) COLOR = COLOR_GREY   ;
      if( severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning ) COLOR = COLOR_YELLOW ;
      if( severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError   ) COLOR = COLOR_RED    ;
      
      if( output_severity != Instance::DebugSeverity::None && output_level != Instance::DebugOutputLevel::Quiet )
      {
        if( toFlags( output_level ) & type && toFlags( output_severity ) & severity ) 
        {
          std::cout << "\n" ;
          std::cout << COLOR << "--  NYX::VKG Instance Debug"                  << END_COLOR << "\n" ;
          std::cout << COLOR << "--  Type    : "  << vk::to_string( type     ) << END_COLOR << "\n" ;
          std::cout << COLOR << "--  Severity: "  << vk::to_string( severity ) << END_COLOR << "\n" ;
          std::cout << COLOR << "---> Message: " << pCallbackData->pMessage    << END_COLOR << "\n" ;
        }
      }
      pUserData = pUserData ;
      return VK_FALSE;
    }
    
    /** Structure to contain a version in major.minor.revision format.
     */
    struct Version
    {
      unsigned major    ; ///< The major revision.
      unsigned minor    ; ///< The minor revision.
      unsigned revision ; ///< The revision.
    };
    
    /** Structure to contain a vulkan instance's data.
     */
    struct InstanceData
    {
      typedef std::vector<std::string>          List       ; ///< A List of std::strings.
      typedef std::vector<const char*>          CharList   ; ///< A List of C-Strings.
      typedef std::vector<::vk::PhysicalDevice> DeviceList ; ///< A List of Vulkan Physical Devices.
      
      ::vk::Instance instance     ; ///< The vulkan image.
      Version        app_version  ; ///< The application version.
      Version        api_version  ; ///< The API version.
      std::string    app_name     ; ///< The name of the application.
      List           ext_list     ; ///< The list of extensions to use for instance generation.
      List           layer_list   ; ///< The list of validation layers to use for instance generation.
      DeviceList     physical_dev ; ///< The list of physical devices found on the system.
      bool           debug        ; ///< Whether or not to generate debug callbacks.

      /** Default constructor.
       */
      InstanceData() ;
              
      /** Method to create the vulkan application info.
       * @return A Filled out Vulkan Application Info.
       */
      ::vk::ApplicationInfo makeAppInfo() const ;
      
      /** Method to create Vulkan debug info object.
       * @return A Filled out Vulkan Debug info object.
       */
      ::vk::DebugUtilsMessengerCreateInfoEXT makeDebugInfo() const ;
      
      /** Method to retrieve a filtered list of available extensions.
       * @return A Valid list of extensions.
       */
      List filterExtensions() const ;
      
      /** Method to retrieve a filtered list of available validation layers.
       * @return A valid list of validiation layers.
       */
      List filterValidationLayers() const ;
      
      /** Method to convert a list of strings to list of char* for vulkan objects.
       * @param list The list to convert.
       * @return A List of const char* pointing to string list's strings.
       */
      CharList listToCharList( List& list ) ;
    };
    
    
    InstanceData::InstanceData()
    {
      app_version = { 0, 0, 1 }        ;
      api_version = { 1, 1, 0 }        ;
      app_name    = "NYX_DEFAULT_NAME" ;
      debug       = true               ;
      this->ext_list = { "VK_KHR_surface", "VK_KHR_surface" } ;
    }
    
    vk::DebugUtilsMessageSeverityFlagsEXT toFlags( Instance::DebugSeverity level )
    {
      using flag = vk::DebugUtilsMessageSeverityFlagBitsEXT ;
      switch( level )
      {
        case Instance::DebugSeverity::ErrorsOnly : return flag::eError                  ;
        case Instance::DebugSeverity::WError     : return flag::eWarning | flag::eError ;
        default : return flag::eInfo ;
      }
    }
    
    vk::DebugUtilsMessageTypeFlagsEXT toFlags( Instance::DebugOutputLevel type )
    {
      using flag = vk::DebugUtilsMessageTypeFlagBitsEXT ;
      switch( type )
      {
        case Instance::DebugOutputLevel::Normal  : return flag::eGeneral                                      ;
        case Instance::DebugOutputLevel::Verbose : return flag::eValidation | flag::eGeneral | flag::eGeneral ;
        default : return flag::eGeneral ;
      }
    }
    
    ::vk::ApplicationInfo InstanceData::makeAppInfo() const
    {
      ::vk::ApplicationInfo info ;
      
      constexpr auto engine_version = VK_MAKE_VERSION( 0, 0, 0                                                                      ) ;
      auto           app_version    = VK_MAKE_VERSION( this->app_version.major, this->app_version.minor, this->app_version.revision ) ;
      auto           api_version    = VK_MAKE_VERSION( this->api_version.major, this->api_version.minor, this->api_version.revision ) ;

      info.setPApplicationName  ( this->app_name.c_str() ) ;
      info.setPEngineName       ( "NYX"                  ) ;
      info.setApplicationVersion( app_version            ) ;
      info.setApiVersion        ( api_version            ) ;
      info.setEngineVersion     ( engine_version         ) ;
      
      return info ;
    }
    
    ::vk::DebugUtilsMessengerCreateInfoEXT InstanceData::makeDebugInfo() const
    {
      const auto severity_bits = ::vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | ::vk::DebugUtilsMessageSeverityFlagBitsEXT::eError  | ::vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning ;
      const auto message_type  = ::vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral     | ::vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | ::vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance ;
      ::vk::DebugUtilsMessengerCreateInfoEXT info ;

      if( this->debug )
      {
        info.setMessageSeverity( severity_bits ) ;
        info.setMessageType    ( message_type  ) ;
        info.setPfnUserCallback( debugCallback ) ;
      }
      
      return info ;
    }
    
    InstanceData::List InstanceData::filterExtensions() const
    {
      List                                   list                 ;
      std::vector<::vk::ExtensionProperties> available_extentions ;
      
      auto result = vk::enumerateInstanceExtensionProperties() ;
      
      vkg::Vulkan::add( result.result ) ;
      available_extentions = result.value ;
      
      for( const auto& ext : available_extentions )
      {
        for( const auto& requested : this->ext_list )
        {
          if( std::string( ext.extensionName.data() ) == std::string( requested ) )
          {
            list.push_back( std::string( ext.extensionName.data() ) ) ;
          }
        }
      }
      
      return list ;
    }
    
    InstanceData::List InstanceData::filterValidationLayers() const
    {
      List                               list             ;
      std::vector<::vk::LayerProperties> available_layers ;
      
      if( this->debug )
      {
        auto result = vk::enumerateInstanceLayerProperties() ;
        vkg::Vulkan::add( result.result ) ;
        available_layers = result.value ;
        
        for( const auto& prop : available_layers )
        {
          for( const auto& requested : this->layer_list )
          {
            if( std::string( prop.layerName.data() ) == std::string( requested ) )
            {
              list.push_back( std::string( prop.layerName.data() ) ) ;
            }
          }
        }
      }
      
      return list ;
    }
    
    InstanceData::CharList InstanceData::listToCharList( List& list )
    {
      InstanceData::CharList char_list ;
      
      for( auto &str : list )
      {
        char_list.push_back( str.c_str() ) ;
      }
      
      return char_list ;
    }

    Instance::Instance()
    {
      this->instance_data = new InstanceData() ;
    }

    Instance::Instance( const Instance& instance )
    {
      this->instance_data = new InstanceData() ;
      
      *this = instance ;
    }

    Instance::~Instance()
    {
      delete this->instance_data ;
    }
    
    unsigned Instance::numDevices() const
    {
      return data().physical_dev.size() ;
    }

    Instance& Instance::operator=( const Instance& instance )
    {
      *this->instance_data = *instance.instance_data ;
      
      return *this ;
    }
    
    Instance::operator const ::vk::Instance&() const
    {
      return data().instance ;
    }    
    
    Instance::operator ::vk::Instance() const
    {
      return data().instance ;
    }
    
    Instance::operator const ::vk::Instance&()
    {
      return data().instance ;
    }
    
    Instance::operator ::vk::Instance()
    {
      return data().instance ;
    }
    
    void Instance::setDebugOutputLevel( Instance::DebugOutputLevel level )
    {
      nyx::vkg::output_level = level ;
    }
    
    void Instance::setDebugOutputType( Instance::DebugSeverity severity )
    {
      nyx::vkg::output_severity = severity ;
    }
        
    void Instance::setApplicationName( const char* app_name )
    {
      data().app_name = app_name ;
    }
    
    const ::vk::Instance& Instance::instance() const
    {
      return data().instance ;
    }

    void Instance::initialize()
    {
      vk::DynamicLoader                    loader                ;
      vk::InstanceCreateInfo               info                  ;
      vk::ApplicationInfo                  app_info              ;
      vk::DebugUtilsMessengerCreateInfoEXT debug_info            ;
      InstanceData::List                   ext_list              ;
      InstanceData::List                   layer_list            ;
      InstanceData::CharList               ext_list_char         ;
      InstanceData::CharList               layer_list_char       ;
      PFN_vkGetInstanceProcAddr            vkGetInstanceProcAddr ;
      
      vkGetInstanceProcAddr = loader.getProcAddress<PFN_vkGetInstanceProcAddr>( "vkGetInstanceProcAddr" ) ;
      VULKAN_HPP_DEFAULT_DISPATCHER.init( vkGetInstanceProcAddr ) ;
      auto instance = vk::createInstance( {}, nullptr ) ;
      VULKAN_HPP_DEFAULT_DISPATCHER.init( instance.value ) ;
      
      app_info   = data().makeAppInfo()            ;
      debug_info = data().makeDebugInfo()          ;
      ext_list   = data().filterExtensions()       ;
      layer_list = data().filterValidationLayers() ;
      
      
      if( data().debug )
      {
        ext_list.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME ) ;
        info.setPNext( reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>( &debug_info ) ) ;
      }
      
      ext_list_char   = data().listToCharList( ext_list )   ;
      layer_list_char = data().listToCharList( layer_list ) ;

      info.setEnabledLayerCount      ( layer_list.size()      ) ;
      info.setPpEnabledLayerNames    ( layer_list_char.data() ) ;
      info.setEnabledExtensionCount  ( ext_list.size()        ) ;
      info.setPpEnabledExtensionNames( ext_list_char.data()   ) ;
      info.setPApplicationInfo       ( &app_info              ) ;
      
      auto result = vk::createInstance( info ) ;
      vkg::Vulkan::add( result.result ) ;
      data().instance = result.value ;
      
      VULKAN_HPP_DEFAULT_DISPATCHER.init( result.value ) ;
      auto result2 = data().instance.enumeratePhysicalDevices() ;
      vkg::Vulkan::add( result2.result ) ;
      data().physical_dev = result2.value ;
      
    }

    void Instance::setApplicationVersion( unsigned major, unsigned minor, unsigned revision )
    {
      data().app_version = { major, minor, revision } ;
    }

    void Instance::setApiVersion( unsigned major, unsigned minor, unsigned revision )
    {
      data().api_version = { major, minor, revision } ;
    }

    void Instance::reset()
    {
      data().instance.destroy() ;
    }
    
    bool Instance::initialized() const
    {
      return data().instance ? true : false ;
    }

    void Instance::addExtension( const char* extension_name )
    {
      data().ext_list.push_back( extension_name ) ;
    }
    
    void Instance::addValidationLayer( const char* layer_name )
    {
      data().layer_list.push_back( layer_name ) ;
    }
    
    const ::vk::PhysicalDevice& Instance::device( unsigned id )
    {
      return id < data().physical_dev.size() ? data().physical_dev[ id ] : data().physical_dev[ 0 ] ;
    }
    
    InstanceData& Instance::data()
    {
      return *this->instance_data ;
    }

    const InstanceData& Instance::data() const
    {
      return *this->instance_data ;
    }
  }
}
