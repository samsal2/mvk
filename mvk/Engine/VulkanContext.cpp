#include "Engine/VulkanContext.hpp"

#include "Engine/AllocatorContext.hpp"
#include "Engine/Debug.hpp"
#include "Utility/Verify.hpp"
#include "vulkan/vulkan_core.h"

namespace Mvk::Engine
{
  [[nodiscard]] VkExtent2D VulkanContext::getFramebufferSize() const noexcept
  {
    auto Width  = 0;
    auto Height = 0;

    do
    {
      glfwGetFramebufferSize( Window, &Width, &Height );
      glfwWaitEvents();
    } while ( Width == 0 || Height == 0 );

    return { static_cast<uint32_t>( Width ), static_cast<uint32_t>( Height ) };
  }

  void VulkanContext::initialize( std::string const & Name, Extent Extent )
  {
    AllocatorContext::the().initialize( {} );

    initWindow( Name, Extent );
    initInstace( Name );
    initDbgMsngr();
    initSurface();
    selectPhysicalDevice();
    selectSurfaceFmt();
    initDevice();
  }

  void VulkanContext::initWindow( std::string const & Name, Extent Extent ) noexcept
  {
    glfwInit();
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

    auto const Callback = []( GLFWwindow * const CallbackWindow, [[maybe_unused]] int const CbWidth, [[maybe_unused]] int const CbHeight )
    {
      auto const User                  = glfwGetWindowUserPointer( CallbackWindow );
      auto const CurrentCtx            = reinterpret_cast<VulkanContext *>( User );
      CurrentCtx->IsFramebufferResized = true;
    };

    Window = glfwCreateWindow( static_cast<int>( Extent.Width ), static_cast<int>( Extent.Height ), Name.c_str(), nullptr, nullptr );

    glfwSetWindowUserPointer( Window, this );
    glfwSetFramebufferSizeCallback( Window, Callback );
  }

  void VulkanContext::initInstace( std::string const & Name ) noexcept
  {
    if constexpr ( UseValidation )
    {
      auto ValidationLayersPropCount = uint32_t( 0 );
      vkEnumerateInstanceLayerProperties( &ValidationLayersPropCount, nullptr );

      auto ValLayerProps = std::vector<VkLayerProperties>( ValidationLayersPropCount );
      vkEnumerateInstanceLayerProperties( &ValidationLayersPropCount, std::data( ValLayerProps ) );

      auto const Found = [&ValLayerProps]
      {
        for ( auto const & ValLayerProp : ValLayerProps )
        {
          for ( auto const ValLayer : ValidationLayers )
          {
            if ( std::strcmp( ValLayer, ValLayerProp.layerName ) == 0 )
            {
              return true;
            }
          }
        }

        return false;
      }();

      MVK_VERIFY( Found );
    }

    auto AppInfo               = VkApplicationInfo();
    AppInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    AppInfo.pApplicationName   = Name.c_str();
    AppInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    AppInfo.pEngineName        = "No Engine";
    AppInfo.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );

    auto       ReqInstExtCount = uint32_t( 0 );
    auto const ReqInstExtData  = glfwGetRequiredInstanceExtensions( &ReqInstExtCount );

    auto ReqExts = std::vector<char const *>( ReqInstExtData, std::next( ReqInstExtData, ReqInstExtCount ) );

    if constexpr ( UseValidation )
    {
      ReqExts.insert( std::begin( ReqExts ), std::begin( ValidationInstanceExtensionss ), std::end( ValidationInstanceExtensionss ) );
    }

    auto InstCrtInfo  = VkInstanceCreateInfo();
    InstCrtInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    if constexpr ( UseValidation )
    {
      InstCrtInfo.pNext = &DbgCrtInfo;
    }
    else
    {
      InstCrtInfo.pNext = nullptr;
    }

    InstCrtInfo.pApplicationInfo = &AppInfo;

    if constexpr ( UseValidation )
    {
      InstCrtInfo.enabledLayerCount   = static_cast<uint32_t>( std::size( ValidationLayers ) );
      InstCrtInfo.ppEnabledLayerNames = std::data( ValidationLayers );
    }
    else
    {
      InstCrtInfo.enabledLayerCount       = 0;
      InstCrtInfo.ppEnabledExtensionNames = nullptr;
    }

    InstCrtInfo.enabledExtensionCount   = static_cast<uint32_t>( std::size( ReqExts ) );
    InstCrtInfo.ppEnabledExtensionNames = std::data( ReqExts );

    auto Result = vkCreateInstance( &InstCrtInfo, nullptr, &Instance );
    MVK_VERIFY( Result == VK_SUCCESS );
  }

  void VulkanContext::initDbgMsngr() noexcept
  {
    if constexpr ( UseValidation )
    {
      auto const CrtDbgUtilMsngr =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>( vkGetInstanceProcAddr( Instance, "vkCreateDebugUtilsMessengerEXT" ) );

      MVK_VERIFY( CrtDbgUtilMsngr );

      CrtDbgUtilMsngr( Instance, &DbgCrtInfo, nullptr, &DbgMsngr );
    }
  }

  void VulkanContext::initSurface() noexcept
  {
    glfwCreateWindowSurface( Instance, Window, nullptr, &Surface );
  }

  void VulkanContext::selectPhysicalDevice() noexcept
  {
    auto AvailablePhysicalDeviceCount = uint32_t( 0 );
    vkEnumeratePhysicalDevices( Instance, &AvailablePhysicalDeviceCount, nullptr );

    auto AvailablePhysicalDevices = std::vector<VkPhysicalDevice>( AvailablePhysicalDeviceCount );
    vkEnumeratePhysicalDevices( Instance, &AvailablePhysicalDeviceCount, std::data( AvailablePhysicalDevices ) );

    for ( auto const AvailablePhysicalDevice : AvailablePhysicalDevices )
    {
      auto features = VkPhysicalDeviceFeatures();
      vkGetPhysicalDeviceFeatures( AvailablePhysicalDevice, &features );

      if ( Detail::chkExtSup( AvailablePhysicalDevice, DeviceExtensions ) &&
           Detail::chkFmtAndPresentModeAvailablity( AvailablePhysicalDevice, Surface ) &&
           Detail::queryFamiliyIdxs( AvailablePhysicalDevice, Surface ).has_value() && features.samplerAnisotropy )
      {
        PhysicalDevice = AvailablePhysicalDevice;
        return;
      }
    }

    MVK_VERIFY_NOT_REACHED();
  }

  void VulkanContext::selectSurfaceFmt() noexcept
  {
    auto FmtCount = uint32_t( 0 );
    vkGetPhysicalDeviceSurfaceFormatsKHR( PhysicalDevice, Surface, &FmtCount, nullptr );

    auto Fmts = std::vector<VkSurfaceFormatKHR>( FmtCount );
    vkGetPhysicalDeviceSurfaceFormatsKHR( PhysicalDevice, Surface, &FmtCount, std::data( Fmts ) );

    for ( auto const Fmt : Fmts )
    {
      if ( Fmt.format == VK_FORMAT_B8G8R8A8_SRGB && Fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
      {
        SurfaceFmt = Fmt;
        return;
      }
    }
  }

  void VulkanContext::initDevice() noexcept
  {
    auto const OptQueueIdx = Detail::queryFamiliyIdxs( PhysicalDevice, Surface );

    MVK_VERIFY( OptQueueIdx.has_value() );

    auto const QueueIdxs = OptQueueIdx.value();
    GfxQueueIdx          = QueueIdxs.first;
    PresentQueueIdx      = QueueIdxs.second;

    auto Features = VkPhysicalDeviceFeatures();
    vkGetPhysicalDeviceFeatures( PhysicalDevice, &Features );

    auto const QueuePrio = 1.0F;

    auto GfxQueueCrtInfo             = VkDeviceQueueCreateInfo();
    GfxQueueCrtInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    GfxQueueCrtInfo.queueFamilyIndex = GfxQueueIdx;
    GfxQueueCrtInfo.queueCount       = 1;
    GfxQueueCrtInfo.pQueuePriorities = &QueuePrio;

    auto PresentQueueCrtInfo             = VkDeviceQueueCreateInfo();
    PresentQueueCrtInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    PresentQueueCrtInfo.queueFamilyIndex = PresentQueueIdx;
    PresentQueueCrtInfo.queueCount       = 1;
    PresentQueueCrtInfo.pQueuePriorities = &QueuePrio;

    auto const queue_create_info       = std::array{ GfxQueueCrtInfo, PresentQueueCrtInfo };
    auto const queue_create_info_count = static_cast<uint32_t>( QueueIdxs.first != QueueIdxs.second ? 2 : 1 );

    auto DeviceCrtInfo                    = VkDeviceCreateInfo();
    DeviceCrtInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceCrtInfo.queueCreateInfoCount    = queue_create_info_count;
    DeviceCrtInfo.pQueueCreateInfos       = std::data( queue_create_info );
    DeviceCrtInfo.pEnabledFeatures        = &Features;
    DeviceCrtInfo.enabledExtensionCount   = static_cast<uint32_t>( std::size( DeviceExtensions ) );
    DeviceCrtInfo.ppEnabledExtensionNames = std::data( DeviceExtensions );

    if constexpr ( UseValidation )
    {
      DeviceCrtInfo.enabledLayerCount   = static_cast<uint32_t>( std::size( ValidationLayers ) );
      DeviceCrtInfo.ppEnabledLayerNames = std::data( ValidationLayers );
    }
    else
    {
      DeviceCrtInfo.enabledLayerCount   = 0;
      DeviceCrtInfo.ppEnabledLayerNames = nullptr;
    }

    auto Result = vkCreateDevice( PhysicalDevice, &DeviceCrtInfo, nullptr, &Device );
    MVK_VERIFY( Result == VK_SUCCESS );

    vkGetDeviceQueue( Device, GfxQueueIdx, 0, &GfxQueue );
    vkGetDeviceQueue( Device, PresentQueueIdx, 0, &PresentQueue );
  }

  void VulkanContext::dstrWindow() noexcept
  {
    glfwDestroyWindow( Window );
    glfwTerminate();
  }

  void VulkanContext::dstrInstance() noexcept
  {
    vkDestroyInstance( Instance, nullptr );
  }

  void VulkanContext::dstrDbgMsngr() noexcept
  {
    if constexpr ( UseValidation )
    {
      auto const DestroyDbgMsngr =
        reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>( vkGetInstanceProcAddr( Instance, "vkDestroyDebugUtilsMessengerEXT" ) );

      MVK_VERIFY( DestroyDbgMsngr );

      DestroyDbgMsngr( Instance, DbgMsngr, nullptr );
    }
  }

  void VulkanContext::dstrSurface() noexcept
  {
    vkDestroySurfaceKHR( Instance, Surface, nullptr );
  }
  void VulkanContext::dstrDevice() noexcept
  {
    vkDestroyDevice( Device, nullptr );
  }

  void VulkanContext::shutdown() noexcept
  {
    AllocatorContext::the().shutdown();
    dstrDevice();
    dstrSurface();
    dstrDbgMsngr();
    dstrInstance();
    dstrWindow();
  }

  [[nodiscard]] float VulkanContext::getCurrentTime() const noexcept
  {
    auto const CurrentTime = std::chrono::high_resolution_clock::now();
    auto const DeltaTime   = CurrentTime - StartTime;
    return std::chrono::duration<float, std::chrono::seconds::period>( DeltaTime ).count();
  }

}  // namespace Mvk::Engine
