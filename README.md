# (K)arma (G)PU (L)ibrary
  KGL is a GPU library aimed to supply a generic interface to interact with the GPU, while also providing specific libraries for API's and OS's for more fine-grained access if needed.

## How to build and install
  KGL is built using **CMake**. To build from source, simply do: 
  
  ```
  mkdir build
  cd build
  cmake ..
  make 
  ```
  
  Generated RPM's install to /usr/local/KGL, and provides a KGLConfig.cmake there to use in any project you make.

  Simply: 
  1) Add the path to KGLConfig.cmake to your *CMAKE_PREFIX_PATH* ( I recommend putting it in your project directory ).
  2) ```FIND_PACKAGE( KGL ) ```
  3) Link against any KGL library ( kgl, kgl_vkg, kgl_linux, etc. ) you need!

## How to use
The core library is intended to be a templated GPGPU library that enables development using whatever platform and API.

Example:
```
// Generic Memory Object
#include <library/Memory.h>

// For vulkan API.
#include <vkg/Vulkan.h>

// For CUDA API.
#include <cg/Cuda.h>

int main()
{
  // Aliases for API's.
  using VULKAN = kgl::vkg::Vulkan ;
  using CUDA   = kgl::cg::Cuda    ;

  kgl::Memory<CUDA  > cuda_memory   ;
  kgl::Memory<VULKAN> vulkan_memory ;
  
  // do things with them here.

  return 0 ;
}
```

```
// For KGL Window.
#include <library/Window.h>

// For vulkan API.
#include <vkg/Vulkan.h>
#include <vkg/Window.h>
// For CUDA API.
#include <cg/Cuda.h>

int main()
{
  // Aliases for API's.
  using VULKAN = kgl::vkg::Vulkan ;
  using CUDA   = kgl::cg::Cuda    ;
 
  // Implicit XCB on Linux or WINAPI on Windows depending on OS.
  kgl::Window<VULKAN> window ;
  
  window.initialize( "Test window", 1024, 720 ) ;

  // Windows supplies a Graphics API-specific context.
  vk::Surface surface = window.context() ;

  return 0 ;
}
```

## Dependancies

 Base, KGL depends on nothing. However, if it can find the libraries on the system, it will build what it can. 
 
 E.g. If vulkan is found, KGL Vulkan libs will be build.

XCB is used for Linux windowing.

Karma Test ( KT ) is used for testing. To output tests, install the KT library. ( See https://github.com/JordanHendl/KT )

### Closing
For usages of each specific API, I suggest checking out the Test.cpp in each one!

If you have any questions or suggestions shoot me an email at jordiehendl@gmail.com
or hit me up on twitter (@jajajordie)
