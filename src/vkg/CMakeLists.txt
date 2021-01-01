FIND_PACKAGE( Vulkan )

IF( ${Vulkan_FOUND} )
    SET( KGL_VULKAN_SOURCES 
         Vulkan.cpp
         Buffer.cpp
         Device.cpp
         Instance.cpp
         Image.cpp
         RenderPass.cpp
         Queue.cpp
         Pipeline.cpp
         CommandBuffer.cpp
         KgShader.cpp
       )

    SET( KGL_VULKAN_HEADERS
         Vulkan.h
         Buffer.h
         Device.h
         Instance.h
         Image.h
         RenderPass.h
         Queue.h
         Pipeline.h
         CommandBuffer.h
         KgShader.h
       )

    SET( KGL_VULKAN_INCLUDE_DIRS
         ${Vulkan_INCLUDE_DIRS}
       )

    SET( KGL_VULKAN_LIBRARIES
         ${Vulkan_LIBRARIES}
        )

    IF( UNIX AND NOT APPLE )
      SET( KGL_VULKAN_LIBRARIES ${KGL_VULKAN_LIBRARIES} kgl_linux )
    ELSEIF( WIN32 )
      SET( KGL_VULKAN_LIBRARIES ${KGL_VULKAN_LIBRARIES} kgl_win32 )
    ENDIF()

    ADD_LIBRARY               ( kgl_vkg SHARED  ${KGL_VULKAN_SOURCES} ${KGL_VULKAN_HEADERS} )
    TARGET_LINK_LIBRARIES     ( kgl_vkg PUBLIC  ${KGL_VULKAN_LIBRARIES}                     )
    TARGET_INCLUDE_DIRECTORIES( kgl_vkg PRIVATE ${KGL_VULKAN_INCLUDE_DIRS}                  )
    
    ADD_EXECUTABLE       ( kgl_vkg_test Test.cpp )
    TARGET_LINK_LIBRARIES( kgl_vkg_test kgl_vkg  )

    INSTALL( FILES  ${KGL_VULKAN_HEADERS} DESTINATION ${INCLUDE_DIR}/vkg )
    INSTALL( TARGETS kgl_vkg EXPORT kgl DESTINATION   ${LIB_DIR}/vkg     )

ENDIF()