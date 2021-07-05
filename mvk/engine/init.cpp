#include "engine/init.hpp"

#include "detail/helpers.hpp"
#include "detail/misc.hpp"
#include "detail/readers.hpp"
#include "engine/Context.hpp"
#include "engine/allocate.hpp"
#include "engine/debug.hpp"
#include "engine/image.hpp"
#include "utility/verify.hpp"

namespace mvk::engine
{
  [[nodiscard]] Context createContext( char const * Name, VkExtent2D Extent ) noexcept
  {
    auto Ctx = Context();
    initWindow( Ctx, Extent );
    initInst( Ctx, Name );
    initDbgMsngr( Ctx );
    initSurf( Ctx );
    selectPhysicalDev( Ctx );
    selectSurfFmt( Ctx );
    initDev( Ctx );
    crtStagingBuffAndMem( Ctx, 1024 * 1024 );
    initLays( Ctx );
    initPools( Ctx );
    initSwapchain( Ctx );
    initDepthImg( Ctx );
    initRdrPass( Ctx );
    initFramebuffers( Ctx );
    initSamplers( Ctx );
    initDoesntBelongHere( Ctx );
    initCmdBuffs( Ctx );
    initShaders( Ctx );
    initPipeline( Ctx );
    initSync( Ctx );
    crtVtxBuffAndMem( Ctx, 1024 * 1024 );
    crtIdxBuffAndMem( Ctx, 1024 * 1024 );
    crtStagingBuffMemAndSets( Ctx, 1024 * 1024 );
    return Ctx;
  }

  void destroyContext( Context & Ctx ) noexcept
  {
    dtyGarbageSets( Ctx );
    dtyGarbageMem( Ctx );
    dtyGarbageBuff( Ctx );
    dtyBuffMemAndSet( Ctx );
    dtyIdxBuffAndMem( Ctx );
    dtyVtxBuffAndMem( Ctx );
    destroySync( Ctx );
    destroyPipelines( Ctx );
    destroyShaders( Ctx );
    destroyCmdBuffs( Ctx );
    destroyDoesntBelongHere( Ctx );
    destroySamplers( Ctx );
    destroyFramebuffers( Ctx );
    destroyRdrPass( Ctx );
    destroyDepthImg( Ctx );
    destroySwapchain( Ctx );
    destroyPools( Ctx );
    destroyLays( Ctx );
    dtyStagingBuffAndMem( Ctx );
    destroyDev( Ctx );
    destroySurf( Ctx );
    destroyDbgMsngr( Ctx );
    destroyInst( Ctx );
    destroyWindow( Ctx );
  }

  void initWindow( Context & Ctx, VkExtent2D Extent ) noexcept
  {
    glfwInit();
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

    auto const Cb =
      []( GLFWwindow * const Window, [[maybe_unused]] int const CbWidth, [[maybe_unused]] int const CbHeight )
    {
      auto const User                = glfwGetWindowUserPointer( Window );
      auto const CurrentCtx          = reinterpret_cast< Context * >( User );
      CurrentCtx->FramebufferResized = true;
    };

    Ctx.Window = glfwCreateWindow(
      static_cast< int >( Extent.width ), static_cast< int >( Extent.height ), "stan loona", nullptr, nullptr );

    glfwSetWindowUserPointer( Ctx.Window, &Ctx );
    glfwSetFramebufferSizeCallback( Ctx.Window, Cb );
  }

  void destroyWindow( Context & Ctx ) noexcept
  {
    glfwDestroyWindow( Ctx.Window );
    glfwTerminate();
  }

  void initInst( Context & Ctx, char const * Name ) noexcept
  {
    if constexpr ( Context::UseVal )
    {
      auto ValLaysPropCnt = uint32_t( 0 );
      vkEnumerateInstanceLayerProperties( &ValLaysPropCnt, nullptr );

      auto ValLayerProps = std::vector< VkLayerProperties >( ValLaysPropCnt );
      vkEnumerateInstanceLayerProperties( &ValLaysPropCnt, std::data( ValLayerProps ) );

      auto const Matched = [ &ValLayerProps ]
      {
        for ( auto const & ValLayserProp : ValLayerProps )
        {
          for ( auto const ValLayser : Context::ValLays )
          {
            if ( std::strcmp( ValLayser, ValLayserProp.layerName ) == 0 )
            {
              return true;
            }
          }
        }
        return false;
      }();

      MVK_VERIFY( Matched );
    }

    auto AppInfo               = VkApplicationInfo();
    AppInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    AppInfo.pApplicationName   = Name;
    AppInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    AppInfo.pEngineName        = "No Engine";
    AppInfo.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );

    auto       ReqInstExtCnt  = uint32_t( 0 );
    auto const ReqInstExtData = glfwGetRequiredInstanceExtensions( &ReqInstExtCnt );

    auto ReqExts = std::vector< char const * >( ReqInstExtData, std::next( ReqInstExtData, ReqInstExtCnt ) );

    if constexpr ( Context::UseVal )
    {
      ReqExts.insert( std::begin( ReqExts ), std::begin( Context::ValInstExts ), std::end( Context::ValInstExts ) );
    }

    auto InstCrtInfo  = VkInstanceCreateInfo();
    InstCrtInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    if constexpr ( Context::UseVal )
    {
      InstCrtInfo.pNext = &DbgCrtInfo;
    }
    else
    {
      InstCrtInfo.pNext = nullptr;
    }

    InstCrtInfo.pApplicationInfo = &AppInfo;

    if constexpr ( Context::UseVal )
    {
      InstCrtInfo.enabledLayerCount   = static_cast< uint32_t >( std::size( Context::ValLays ) );
      InstCrtInfo.ppEnabledLayerNames = std::data( Context::ValLays );
    }
    else
    {
      InstCrtInfo.enabledLayerCount       = 0;
      InstCrtInfo.ppEnabledExtensionNames = nullptr;
    }

    InstCrtInfo.enabledExtensionCount   = static_cast< uint32_t >( std::size( ReqExts ) );
    InstCrtInfo.ppEnabledExtensionNames = std::data( ReqExts );

    [[maybe_unused]] auto Result = vkCreateInstance( &InstCrtInfo, nullptr, &Ctx.Inst );
    MVK_VERIFY( Result == VK_SUCCESS );
  }

  void destroyInst( Context & Ctx ) noexcept
  {
    vkDestroyInstance( Ctx.Inst, nullptr );
  }

  void initDbgMsngr( Context & Ctx ) noexcept
  {
    if constexpr ( Context::UseVal )
    {
      auto const CrtDbgUtilMsngr = reinterpret_cast< PFN_vkCreateDebugUtilsMessengerEXT >(
        vkGetInstanceProcAddr( Ctx.Inst, "vkCreateDebugUtilsMessengerEXT" ) );

      MVK_VERIFY( CrtDbgUtilMsngr );

      CrtDbgUtilMsngr( Ctx.Inst, &DbgCrtInfo, nullptr, &Ctx.DbgMsngr );
    }
  }

  void destroyDbgMsngr( Context & Ctx ) noexcept
  {
    if constexpr ( Context::UseVal )
    {
      auto const DestroyDbgMsngr = reinterpret_cast< PFN_vkDestroyDebugUtilsMessengerEXT >(
        vkGetInstanceProcAddr( Ctx.Inst, "vkDestroyDebugUtilsMessengerEXT" ) );

      MVK_VERIFY( DestroyDbgMsngr );

      DestroyDbgMsngr( Ctx.Inst, Ctx.DbgMsngr, nullptr );
    }
  }

  void initSurf( Context & Ctx ) noexcept
  {
    glfwCreateWindowSurface( Ctx.Inst, Ctx.Window, nullptr, &Ctx.Surf );
  }

  void destroySurf( Context & Ctx ) noexcept
  {
    vkDestroySurfaceKHR( Ctx.Inst, Ctx.Surf, nullptr );
  }

  void selectPhysicalDev( Context & Ctx ) noexcept
  {
    auto PhysicalDeviceCnt = uint32_t( 0 );
    vkEnumeratePhysicalDevices( Ctx.Inst, &PhysicalDeviceCnt, nullptr );

    auto PhysicalDevices = std::vector< VkPhysicalDevice >( PhysicalDeviceCnt );
    vkEnumeratePhysicalDevices( Ctx.Inst, &PhysicalDeviceCnt, std::data( PhysicalDevices ) );

    for ( auto const PhysicalDevice : PhysicalDevices )
    {
      auto features = VkPhysicalDeviceFeatures();
      vkGetPhysicalDeviceFeatures( PhysicalDevice, &features );

      if ( detail::chkExtSup( PhysicalDevice, Context::DevExts ) &&
           detail::chkFmtAndPresentModeAvailablity( PhysicalDevice, Ctx.Surf ) &&
           detail::queryFamiliyIdxs( PhysicalDevice, Ctx.Surf ).has_value() && features.samplerAnisotropy )
      {
        Ctx.PhysicalDev = PhysicalDevice;
        return;
      }
    }

    MVK_VERIFY_NOT_REACHED();
  }

  void selectSurfFmt( Context & Ctx ) noexcept
  {
    auto FmtCnt = uint32_t( 0 );
    vkGetPhysicalDeviceSurfaceFormatsKHR( Ctx.PhysicalDev, Ctx.Surf, &FmtCnt, nullptr );

    auto Fmts = std::vector< VkSurfaceFormatKHR >( FmtCnt );
    vkGetPhysicalDeviceSurfaceFormatsKHR( Ctx.PhysicalDev, Ctx.Surf, &FmtCnt, std::data( Fmts ) );

    for ( auto const Fmt : Fmts )
    {
      if ( Fmt.format == VK_FORMAT_B8G8R8A8_SRGB && Fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
      {
        Ctx.SurfFmt = Fmt;
        return;
      }
    }

    Ctx.SurfFmt = Fmts[ 0 ];
  }

  void initDev( Context & Ctx ) noexcept
  {
    auto const OptQueueIdx = detail::queryFamiliyIdxs( Ctx.PhysicalDev, Ctx.Surf );

    MVK_VERIFY( OptQueueIdx.has_value() );

    auto const QueueIdxs = OptQueueIdx.value();
    Ctx.GfxQueueIdx      = QueueIdxs.first;
    Ctx.PresentQueueIdx  = QueueIdxs.second;

    auto Features = VkPhysicalDeviceFeatures();
    vkGetPhysicalDeviceFeatures( Ctx.PhysicalDev, &Features );

    auto const QueuePrio = 1.0F;

    auto GfxQueueCrtInfo             = VkDeviceQueueCreateInfo();
    GfxQueueCrtInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    GfxQueueCrtInfo.queueFamilyIndex = Ctx.GfxQueueIdx;
    GfxQueueCrtInfo.queueCount       = 1;
    GfxQueueCrtInfo.pQueuePriorities = &QueuePrio;

    auto PresentQueueCrtInfo             = VkDeviceQueueCreateInfo();
    PresentQueueCrtInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    PresentQueueCrtInfo.queueFamilyIndex = Ctx.PresentQueueIdx;
    PresentQueueCrtInfo.queueCount       = 1;
    PresentQueueCrtInfo.pQueuePriorities = &QueuePrio;

    auto const queue_create_info       = std::array{ GfxQueueCrtInfo, PresentQueueCrtInfo };
    auto const queue_create_info_count = static_cast< uint32_t >( QueueIdxs.first != QueueIdxs.second ? 2 : 1 );

    auto Device_create_info                    = VkDeviceCreateInfo();
    Device_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    Device_create_info.queueCreateInfoCount    = queue_create_info_count;
    Device_create_info.pQueueCreateInfos       = std::data( queue_create_info );
    Device_create_info.pEnabledFeatures        = &Features;
    Device_create_info.enabledExtensionCount   = static_cast< uint32_t >( std::size( Context::DevExts ) );
    Device_create_info.ppEnabledExtensionNames = std::data( Context::DevExts );

    if constexpr ( Context::UseVal )
    {
      Device_create_info.enabledLayerCount   = static_cast< uint32_t >( std::size( Context::ValLays ) );
      Device_create_info.ppEnabledLayerNames = std::data( Context::ValLays );
    }
    else
    {
      Device_create_info.enabledLayerCount   = 0;
      Device_create_info.ppEnabledLayerNames = nullptr;
    }

    [[maybe_unused]] auto Result = vkCreateDevice( Ctx.PhysicalDev, &Device_create_info, nullptr, &Ctx.Dev );
    MVK_VERIFY( Result == VK_SUCCESS );

    vkGetDeviceQueue( Ctx.Dev, Ctx.GfxQueueIdx, 0, &Ctx.GfxQueue );
    vkGetDeviceQueue( Ctx.Dev, Ctx.PresentQueueIdx, 0, &Ctx.PresentQueue );
  }

  void destroyDev( Context & Ctx ) noexcept
  {
    vkDestroyDevice( Ctx.Dev, nullptr );
  }

  void initLays( Context & Ctx ) noexcept
  {
    auto UniformDescriptorSetLayBind               = VkDescriptorSetLayoutBinding();
    UniformDescriptorSetLayBind.binding            = 0;
    UniformDescriptorSetLayBind.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    UniformDescriptorSetLayBind.descriptorCount    = 1;
    UniformDescriptorSetLayBind.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    UniformDescriptorSetLayBind.pImmutableSamplers = nullptr;

    auto UniformDescriptorSetLayoutCrtInfo         = VkDescriptorSetLayoutCreateInfo();
    UniformDescriptorSetLayoutCrtInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    UniformDescriptorSetLayoutCrtInfo.bindingCount = 1;
    UniformDescriptorSetLayoutCrtInfo.pBindings    = &UniformDescriptorSetLayBind;

    auto Result =
      vkCreateDescriptorSetLayout( Ctx.Dev, &UniformDescriptorSetLayoutCrtInfo, nullptr, &Ctx.UboDescriptorSetLay );

    MVK_VERIFY( Result == VK_SUCCESS );

    auto SamplerDescriptorSetLayBind               = VkDescriptorSetLayoutBinding();
    SamplerDescriptorSetLayBind.binding            = 0;
    SamplerDescriptorSetLayBind.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    SamplerDescriptorSetLayBind.descriptorCount    = 1;
    SamplerDescriptorSetLayBind.pImmutableSamplers = nullptr;
    SamplerDescriptorSetLayBind.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

    auto SamplerDescriptorSetLayCrtInfo         = VkDescriptorSetLayoutCreateInfo();
    SamplerDescriptorSetLayCrtInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    SamplerDescriptorSetLayCrtInfo.bindingCount = 1;
    SamplerDescriptorSetLayCrtInfo.pBindings    = &SamplerDescriptorSetLayBind;

    Result = vkCreateDescriptorSetLayout( Ctx.Dev, &SamplerDescriptorSetLayCrtInfo, nullptr, &Ctx.TexDescriptorSetLay );

    MVK_VERIFY( Result == VK_SUCCESS );

    auto DescriptorSetLays = std::array{ Ctx.UboDescriptorSetLay, Ctx.TexDescriptorSetLay };

    auto PipelineLayCrtInfo                   = VkPipelineLayoutCreateInfo();
    PipelineLayCrtInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayCrtInfo.setLayoutCount         = static_cast< uint32_t >( std::size( DescriptorSetLays ) );
    PipelineLayCrtInfo.pSetLayouts            = std::data( DescriptorSetLays );
    PipelineLayCrtInfo.pushConstantRangeCount = 0;
    PipelineLayCrtInfo.pPushConstantRanges    = nullptr;

    Result = vkCreatePipelineLayout( Ctx.Dev, &PipelineLayCrtInfo, nullptr, &Ctx.PipelineLay );

    MVK_VERIFY( Result == VK_SUCCESS );
  }

  void destroyLays( Context & Ctx ) noexcept
  {
    vkDestroyPipelineLayout( Ctx.Dev, Ctx.PipelineLay, nullptr );
    vkDestroyDescriptorSetLayout( Ctx.Dev, Ctx.TexDescriptorSetLay, nullptr );
    vkDestroyDescriptorSetLayout( Ctx.Dev, Ctx.UboDescriptorSetLay, nullptr );
  }

  void initPools( Context & Ctx ) noexcept
  {
    auto CmdPoolCrtInfo             = VkCommandPoolCreateInfo();
    CmdPoolCrtInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    CmdPoolCrtInfo.queueFamilyIndex = Ctx.GfxQueueIdx;
    CmdPoolCrtInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    [[maybe_unused]] auto Result = vkCreateCommandPool( Ctx.Dev, &CmdPoolCrtInfo, nullptr, &Ctx.CmdPool );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto UniformDescriptorPoolSize            = VkDescriptorPoolSize();
    UniformDescriptorPoolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    UniformDescriptorPoolSize.descriptorCount = 32;

    auto SamplerDescriptorPoolSize            = VkDescriptorPoolSize();
    SamplerDescriptorPoolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    SamplerDescriptorPoolSize.descriptorCount = 32;

    auto const DescriptorPoolSizes = std::array{ UniformDescriptorPoolSize, SamplerDescriptorPoolSize };

    auto DescriptorPoolCrtInfo          = VkDescriptorPoolCreateInfo();
    DescriptorPoolCrtInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    DescriptorPoolCrtInfo.poolSizeCount = static_cast< uint32_t >( std::size( DescriptorPoolSizes ) );
    DescriptorPoolCrtInfo.pPoolSizes    = std::data( DescriptorPoolSizes );
    DescriptorPoolCrtInfo.maxSets       = 128;
    DescriptorPoolCrtInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    vkCreateDescriptorPool( Ctx.Dev, &DescriptorPoolCrtInfo, nullptr, &Ctx.DescriptorPool );
  }

  void destroyPools( Context & Ctx ) noexcept
  {
    vkDestroyDescriptorPool( Ctx.Dev, Ctx.DescriptorPool, nullptr );
    vkDestroyCommandPool( Ctx.Dev, Ctx.CmdPool, nullptr );
  }

  void initSwapchain( Context & Ctx ) noexcept
  {
    auto const FamilyIdxs      = std::array{ Ctx.GfxQueueIdx, Ctx.PresentQueueIdx };
    auto const FramebufferSize = queryFramebufferSize( Ctx );

    auto Capabilities = VkSurfaceCapabilitiesKHR();
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( Ctx.PhysicalDev, Ctx.Surf, &Capabilities );

    auto const present_mode = detail::choosePresentMode( Ctx.PhysicalDev, Ctx.Surf );
    Ctx.SwapchainExtent     = detail::chooseExtent( Capabilities, FramebufferSize );
    auto const image_count  = detail::chooseImgCnt( Capabilities );

    auto SwapchainCrtInfo             = VkSwapchainCreateInfoKHR();
    SwapchainCrtInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    SwapchainCrtInfo.surface          = Ctx.Surf;
    SwapchainCrtInfo.minImageCount    = image_count;
    SwapchainCrtInfo.imageFormat      = Ctx.SurfFmt.format;
    SwapchainCrtInfo.imageColorSpace  = Ctx.SurfFmt.colorSpace;
    SwapchainCrtInfo.imageExtent      = Ctx.SwapchainExtent;
    SwapchainCrtInfo.imageArrayLayers = 1;
    SwapchainCrtInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    SwapchainCrtInfo.preTransform     = Capabilities.currentTransform;
    SwapchainCrtInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    SwapchainCrtInfo.presentMode      = present_mode;
    SwapchainCrtInfo.clipped          = VK_TRUE;
    SwapchainCrtInfo.oldSwapchain     = nullptr;

    if ( FamilyIdxs[ 0 ] != FamilyIdxs[ 1 ] )
    {
      SwapchainCrtInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
      SwapchainCrtInfo.queueFamilyIndexCount = 2;
      SwapchainCrtInfo.pQueueFamilyIndices   = std::data( FamilyIdxs );
    }
    else
    {
      SwapchainCrtInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
      SwapchainCrtInfo.queueFamilyIndexCount = 0;
      SwapchainCrtInfo.pQueueFamilyIndices   = nullptr;
    }

    auto Result = vkCreateSwapchainKHR( Ctx.Dev, &SwapchainCrtInfo, nullptr, &Ctx.Swapchain );
    MVK_VERIFY( Result == VK_SUCCESS );

    Result = vkGetSwapchainImagesKHR( Ctx.Dev, Ctx.Swapchain, &Ctx.SwapchainImgCnt, nullptr );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto SwapchainImgs = std::vector< VkImage >( Ctx.SwapchainImgCnt );
    vkGetSwapchainImagesKHR( Ctx.Dev, Ctx.Swapchain, &Ctx.SwapchainImgCnt, std::data( SwapchainImgs ) );

    Ctx.SwapchainImgViews.reserve( Ctx.SwapchainImgCnt );

    for ( auto const image : SwapchainImgs )
    {
      auto SwapchainImgViewCrtInfo                            = VkImageViewCreateInfo();
      SwapchainImgViewCrtInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      SwapchainImgViewCrtInfo.image                           = image;
      SwapchainImgViewCrtInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
      SwapchainImgViewCrtInfo.format                          = SwapchainCrtInfo.imageFormat;
      SwapchainImgViewCrtInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      SwapchainImgViewCrtInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      SwapchainImgViewCrtInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      SwapchainImgViewCrtInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      SwapchainImgViewCrtInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      SwapchainImgViewCrtInfo.subresourceRange.baseMipLevel   = 0;
      SwapchainImgViewCrtInfo.subresourceRange.levelCount     = 1;
      SwapchainImgViewCrtInfo.subresourceRange.baseArrayLayer = 0;
      SwapchainImgViewCrtInfo.subresourceRange.layerCount     = 1;

      auto ImgView = VkImageView();
      Result       = vkCreateImageView( Ctx.Dev, &SwapchainImgViewCrtInfo, nullptr, &ImgView );
      Ctx.SwapchainImgViews.push_back( ImgView );
    }
  }

  void destroySwapchain( Context & Ctx ) noexcept
  {
    for ( auto const ImgView : Ctx.SwapchainImgViews )
    {
      vkDestroyImageView( Ctx.Dev, ImgView, nullptr );
    }

    vkDestroySwapchainKHR( Ctx.Dev, Ctx.Swapchain, nullptr );
  }

  void initDepthImg( Context & Ctx ) noexcept
  {
    auto DepthImgCreateInfo          = VkImageCreateInfo();
    DepthImgCreateInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    DepthImgCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
    DepthImgCreateInfo.extent.width  = Ctx.SwapchainExtent.width;
    DepthImgCreateInfo.extent.height = Ctx.SwapchainExtent.height;
    DepthImgCreateInfo.extent.depth  = 1;
    DepthImgCreateInfo.mipLevels     = 1;
    DepthImgCreateInfo.arrayLayers   = 1;
    DepthImgCreateInfo.format        = VK_FORMAT_D32_SFLOAT;
    DepthImgCreateInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    DepthImgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    DepthImgCreateInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    DepthImgCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    DepthImgCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    DepthImgCreateInfo.flags         = 0;

    auto Result = vkCreateImage( Ctx.Dev, &DepthImgCreateInfo, nullptr, &Ctx.DepthImg );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto DepthImgReq = VkMemoryRequirements();
    vkGetImageMemoryRequirements( Ctx.Dev, Ctx.DepthImg, &DepthImgReq );

    auto const MemTypeIdx =
      detail::queryMemType( Ctx.PhysicalDev, DepthImgReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    MVK_VERIFY( MemTypeIdx.has_value() );

    auto DepthImgMemAllocInfo            = VkMemoryAllocateInfo();
    DepthImgMemAllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    DepthImgMemAllocInfo.allocationSize  = DepthImgReq.size;
    DepthImgMemAllocInfo.memoryTypeIndex = MemTypeIdx.value();

    Result = vkAllocateMemory( Ctx.Dev, &DepthImgMemAllocInfo, nullptr, &Ctx.DepthImgMem );
    MVK_VERIFY( Result == VK_SUCCESS );

    Result = vkBindImageMemory( Ctx.Dev, Ctx.DepthImg, Ctx.DepthImgMem, 0 );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto DepthImgViewCrtInfo                            = VkImageViewCreateInfo();
    DepthImgViewCrtInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    DepthImgViewCrtInfo.image                           = Ctx.DepthImg;
    DepthImgViewCrtInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    DepthImgViewCrtInfo.format                          = VK_FORMAT_D32_SFLOAT;
    DepthImgViewCrtInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    DepthImgViewCrtInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    DepthImgViewCrtInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    DepthImgViewCrtInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    DepthImgViewCrtInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    DepthImgViewCrtInfo.subresourceRange.baseMipLevel   = 0;
    DepthImgViewCrtInfo.subresourceRange.levelCount     = 1;
    DepthImgViewCrtInfo.subresourceRange.baseArrayLayer = 0;
    DepthImgViewCrtInfo.subresourceRange.layerCount     = 1;

    Result = vkCreateImageView( Ctx.Dev, &DepthImgViewCrtInfo, nullptr, &Ctx.DepthImgView );

    MVK_VERIFY( Result == VK_SUCCESS );

    transition_layout(
      Ctx, Ctx.DepthImg, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1 );
  }

  void destroyDepthImg( Context & Ctx ) noexcept
  {
    vkDestroyImageView( Ctx.Dev, Ctx.DepthImgView, nullptr );
    vkFreeMemory( Ctx.Dev, Ctx.DepthImgMem, nullptr );
    vkDestroyImage( Ctx.Dev, Ctx.DepthImg, nullptr );
  }

  void initFramebuffers( Context & Ctx ) noexcept
  {
    Ctx.Framebuffers.reserve( Ctx.SwapchainImgCnt );

    for ( auto const ImgView : Ctx.SwapchainImgViews )
    {
      auto const Attachments = std::array{ ImgView, Ctx.DepthImgView };

      auto FramebufferCrtInfo            = VkFramebufferCreateInfo();
      FramebufferCrtInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      FramebufferCrtInfo.renderPass      = Ctx.RdrPass;
      FramebufferCrtInfo.attachmentCount = static_cast< uint32_t >( std::size( Attachments ) );
      FramebufferCrtInfo.pAttachments    = std::data( Attachments );
      FramebufferCrtInfo.width           = Ctx.SwapchainExtent.width;
      FramebufferCrtInfo.height          = Ctx.SwapchainExtent.height;
      FramebufferCrtInfo.layers          = 1;

      auto Framebuffer = VkFramebuffer();

      [[maybe_unused]] auto Result = vkCreateFramebuffer( Ctx.Dev, &FramebufferCrtInfo, nullptr, &Framebuffer );
      Ctx.Framebuffers.push_back( Framebuffer );
      MVK_VERIFY( Result == VK_SUCCESS );
    }
  }

  void destroyFramebuffers( Context & Ctx ) noexcept
  {
    for ( auto const Framebuffer : Ctx.Framebuffers )

      vkDestroyFramebuffer( Ctx.Dev, Framebuffer, nullptr );
  }

  void initRdrPass( Context & Ctx ) noexcept
  {
    auto ColorAttachDesc           = VkAttachmentDescription();
    ColorAttachDesc.format         = Ctx.SurfFmt.format;
    ColorAttachDesc.samples        = VK_SAMPLE_COUNT_1_BIT;
    ColorAttachDesc.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ColorAttachDesc.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    ColorAttachDesc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ColorAttachDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ColorAttachDesc.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    ColorAttachDesc.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    auto DepthAttachDesc           = VkAttachmentDescription();
    DepthAttachDesc.format         = VK_FORMAT_D32_SFLOAT;
    DepthAttachDesc.samples        = VK_SAMPLE_COUNT_1_BIT;
    DepthAttachDesc.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    DepthAttachDesc.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    DepthAttachDesc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    DepthAttachDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    DepthAttachDesc.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    DepthAttachDesc.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    auto ColorAttachRef       = VkAttachmentReference();
    ColorAttachRef.attachment = 0;
    ColorAttachRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    auto DepthAttachRef       = VkAttachmentReference();
    DepthAttachRef.attachment = 1;
    DepthAttachRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    auto SubpassDesc                    = VkSubpassDescription();
    SubpassDesc.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    SubpassDesc.colorAttachmentCount    = 1;
    SubpassDesc.pColorAttachments       = &ColorAttachRef;
    SubpassDesc.pDepthStencilAttachment = &DepthAttachRef;

    auto SubpassDep          = VkSubpassDependency();
    SubpassDep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    SubpassDep.dstSubpass    = 0;
    SubpassDep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDep.srcAccessMask = 0;
    SubpassDep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    auto const Attachments = std::array{ ColorAttachDesc, DepthAttachDesc };

    auto RdrPassCrtInfo            = VkRenderPassCreateInfo();
    RdrPassCrtInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RdrPassCrtInfo.attachmentCount = static_cast< uint32_t >( std::size( Attachments ) );
    RdrPassCrtInfo.pAttachments    = std::data( Attachments );
    RdrPassCrtInfo.subpassCount    = 1;
    RdrPassCrtInfo.pSubpasses      = &SubpassDesc;
    RdrPassCrtInfo.dependencyCount = 1;
    RdrPassCrtInfo.pDependencies   = &SubpassDep;

    [[maybe_unused]] auto Result = vkCreateRenderPass( Ctx.Dev, &RdrPassCrtInfo, nullptr, &Ctx.RdrPass );
    MVK_VERIFY( Result == VK_SUCCESS );
  }

  void destroyRdrPass( Context & Ctx ) noexcept
  {
    vkDestroyRenderPass( Ctx.Dev, Ctx.RdrPass, nullptr );
  }

  void initDoesntBelongHere( Context & Ctx ) noexcept
  {
    std::tie( Ctx.texture_, Ctx.width_, Ctx.height_ ) = detail::loadTex( "../../assets/viking_room.png" );

    auto ImgCrtInfo          = VkImageCreateInfo();
    ImgCrtInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImgCrtInfo.imageType     = VK_IMAGE_TYPE_2D;
    ImgCrtInfo.extent.width  = Ctx.width_;
    ImgCrtInfo.extent.height = Ctx.height_;
    ImgCrtInfo.extent.depth  = 1;
    ImgCrtInfo.mipLevels     = detail::calcMipLvl( Ctx.width_, Ctx.height_ );
    ImgCrtInfo.arrayLayers   = 1;
    ImgCrtInfo.format        = VK_FORMAT_R8G8B8A8_SRGB;
    ImgCrtInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ImgCrtInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImgCrtInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ImgCrtInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ImgCrtInfo.samples     = VK_SAMPLE_COUNT_1_BIT;
    ImgCrtInfo.flags       = 0;

    auto Result = vkCreateImage( Ctx.Dev, &ImgCrtInfo, nullptr, &Ctx.image_ );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto ImgMemReq = VkMemoryRequirements();
    vkGetImageMemoryRequirements( Ctx.Dev, Ctx.image_, &ImgMemReq );

    auto const MemTypeIdx =
      detail::queryMemType( Ctx.PhysicalDev, ImgMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    MVK_VERIFY( MemTypeIdx.has_value() );

    auto ImgMemAllocInfo            = VkMemoryAllocateInfo();
    ImgMemAllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ImgMemAllocInfo.allocationSize  = ImgMemReq.size;
    ImgMemAllocInfo.memoryTypeIndex = MemTypeIdx.value();

    Result = vkAllocateMemory( Ctx.Dev, &ImgMemAllocInfo, nullptr, &Ctx.image_memory_ );
    MVK_VERIFY( Result == VK_SUCCESS );

    vkBindImageMemory( Ctx.Dev, Ctx.image_, Ctx.image_memory_, 0 );

    transition_layout(
      Ctx, Ctx.image_, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ImgCrtInfo.mipLevels );

    auto StagedTex = allocStaging( Ctx, utility::as_bytes( Ctx.texture_ ) );
    stage_image( Ctx, StagedTex, Ctx.width_, Ctx.height_, Ctx.image_ );
    generate_mipmaps( Ctx, Ctx.image_, Ctx.width_, Ctx.height_, ImgCrtInfo.mipLevels );

    auto ImgViewCrtInfo                            = VkImageViewCreateInfo();
    ImgViewCrtInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImgViewCrtInfo.image                           = Ctx.image_;
    ImgViewCrtInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    ImgViewCrtInfo.format                          = VK_FORMAT_R8G8B8A8_SRGB;
    ImgViewCrtInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImgViewCrtInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImgViewCrtInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImgViewCrtInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImgViewCrtInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    ImgViewCrtInfo.subresourceRange.baseMipLevel   = 0;
    ImgViewCrtInfo.subresourceRange.levelCount     = ImgCrtInfo.mipLevels;
    ImgViewCrtInfo.subresourceRange.baseArrayLayer = 0;
    ImgViewCrtInfo.subresourceRange.layerCount     = 1;

    Result = vkCreateImageView( Ctx.Dev, &ImgViewCrtInfo, nullptr, &Ctx.image_view_ );
    MVK_VERIFY( Result == VK_SUCCESS );

    std::tie( Ctx.vertices_, Ctx.indices_ ) = detail::readObj( "../../assets/viking_room.obj" );

    Ctx.image_descriptor_set_ = allocDescriptorSets< 1 >( Ctx, Ctx.TexDescriptorSetLay )[ 0 ];

    auto ImgDescriptorImgCreateInfo        = VkDescriptorImageInfo();
    ImgDescriptorImgCreateInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ImgDescriptorImgCreateInfo.imageView   = Ctx.image_view_;
    ImgDescriptorImgCreateInfo.sampler     = Ctx.TexSampler;

    auto ImgWriteDescriptorSet             = VkWriteDescriptorSet();
    ImgWriteDescriptorSet.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    ImgWriteDescriptorSet.dstSet           = Ctx.image_descriptor_set_;
    ImgWriteDescriptorSet.dstBinding       = 0;
    ImgWriteDescriptorSet.dstArrayElement  = 0;
    ImgWriteDescriptorSet.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ImgWriteDescriptorSet.descriptorCount  = 1;
    ImgWriteDescriptorSet.pBufferInfo      = nullptr;
    ImgWriteDescriptorSet.pImageInfo       = &ImgDescriptorImgCreateInfo;
    ImgWriteDescriptorSet.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets( Ctx.Dev, 1, &ImgWriteDescriptorSet, 0, nullptr );
  }

  void destroyDoesntBelongHere( Context & Ctx ) noexcept
  {
    vkFreeDescriptorSets( Ctx.Dev, Ctx.DescriptorPool, 1, &Ctx.image_descriptor_set_ );
    vkDestroyImageView( Ctx.Dev, Ctx.image_view_, nullptr );
    vkFreeMemory( Ctx.Dev, Ctx.image_memory_, nullptr );
    vkDestroyImage( Ctx.Dev, Ctx.image_, nullptr );
  }

  void initCmdBuffs( Context & Ctx ) noexcept
  {
    Ctx.CmdBuffs = allocCmdBuff< Context::DynamicBuffCnt >( Ctx, VK_COMMAND_BUFFER_LEVEL_PRIMARY );
  }

  void destroyCmdBuffs( Context & Ctx ) noexcept
  {
    vkFreeCommandBuffers(
      Ctx.Dev, Ctx.CmdPool, static_cast< uint32_t >( std::size( Ctx.CmdBuffs ) ), std::data( Ctx.CmdBuffs ) );
  }

  void initShaders( Context & Ctx ) noexcept
  {
    auto const VtxCode = detail::readFile( "../../shaders/vert.spv" );

    auto VtxShaderModuleCrtInfo     = VkShaderModuleCreateInfo();
    VtxShaderModuleCrtInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    VtxShaderModuleCrtInfo.codeSize = static_cast< uint32_t >( std::size( VtxCode ) );
    VtxShaderModuleCrtInfo.pCode    = reinterpret_cast< uint32_t const * >( std::data( VtxCode ) );

    auto Result = vkCreateShaderModule( Ctx.Dev, &VtxShaderModuleCrtInfo, nullptr, &Ctx.VtxShader );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto const FragCode = detail::readFile( "../../shaders/frag.spv" );

    auto FragShaderModuleCrtInfo     = VkShaderModuleCreateInfo();
    FragShaderModuleCrtInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    FragShaderModuleCrtInfo.codeSize = static_cast< uint32_t >( std::size( FragCode ) );
    FragShaderModuleCrtInfo.pCode    = reinterpret_cast< uint32_t const * >( std::data( FragCode ) );

    Result = vkCreateShaderModule( Ctx.Dev, &FragShaderModuleCrtInfo, nullptr, &Ctx.FragShader );
    MVK_VERIFY( Result == VK_SUCCESS );
  }

  void destroyShaders( Context & Ctx ) noexcept
  {
    vkDestroyShaderModule( Ctx.Dev, Ctx.FragShader, nullptr );
    vkDestroyShaderModule( Ctx.Dev, Ctx.VtxShader, nullptr );
  }

  void initSamplers( Context & Ctx ) noexcept
  {
    auto SamplerCrtInfo                    = VkSamplerCreateInfo();
    SamplerCrtInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCrtInfo.magFilter               = VK_FILTER_LINEAR;
    SamplerCrtInfo.minFilter               = VK_FILTER_LINEAR;
    SamplerCrtInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerCrtInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerCrtInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerCrtInfo.anisotropyEnable        = VK_TRUE;
    SamplerCrtInfo.anisotropyEnable        = VK_TRUE;
    SamplerCrtInfo.maxAnisotropy           = 16;
    SamplerCrtInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    SamplerCrtInfo.unnormalizedCoordinates = VK_FALSE;
    SamplerCrtInfo.compareEnable           = VK_FALSE;
    SamplerCrtInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    SamplerCrtInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerCrtInfo.mipLodBias              = 0.0F;
    SamplerCrtInfo.minLod                  = 0.0F;
    SamplerCrtInfo.maxLod                  = std::numeric_limits< float >::max();

    [[maybe_unused]] auto Result = vkCreateSampler( Ctx.Dev, &SamplerCrtInfo, nullptr, &Ctx.TexSampler );
    MVK_VERIFY( Result == VK_SUCCESS );
  }

  void destroySamplers( Context & Ctx ) noexcept
  {
    vkDestroySampler( Ctx.Dev, Ctx.TexSampler, nullptr );
  }

  void initPipeline( Context & Ctx ) noexcept
  {
    auto VtxInputBindDesc      = VkVertexInputBindingDescription();
    VtxInputBindDesc.binding   = 0;
    VtxInputBindDesc.stride    = sizeof( vertex );
    VtxInputBindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    auto PosVtxInputAttrDesc     = VkVertexInputAttributeDescription();
    PosVtxInputAttrDesc.binding  = 0;
    PosVtxInputAttrDesc.location = 0;
    PosVtxInputAttrDesc.format   = VK_FORMAT_R32G32B32_SFLOAT;
    PosVtxInputAttrDesc.offset   = offsetof( vertex, pos );

    auto ColorVtxInputAttrDesc     = VkVertexInputAttributeDescription();
    ColorVtxInputAttrDesc.binding  = 0;
    ColorVtxInputAttrDesc.location = 1;
    ColorVtxInputAttrDesc.format   = VK_FORMAT_R32G32B32_SFLOAT;
    ColorVtxInputAttrDesc.offset   = offsetof( vertex, color );

    auto TexCoordVtxInputAttrDesc     = VkVertexInputAttributeDescription();
    TexCoordVtxInputAttrDesc.binding  = 0;
    TexCoordVtxInputAttrDesc.location = 2;
    TexCoordVtxInputAttrDesc.format   = VK_FORMAT_R32G32_SFLOAT;
    TexCoordVtxInputAttrDesc.offset   = offsetof( vertex, texture_coord );

    auto const VtxAttrs = std::array{ PosVtxInputAttrDesc, ColorVtxInputAttrDesc, TexCoordVtxInputAttrDesc };

    auto PipelineVtxInputStateCrtInfo  = VkPipelineVertexInputStateCreateInfo();
    PipelineVtxInputStateCrtInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    PipelineVtxInputStateCrtInfo.vertexBindingDescriptionCount   = 1;
    PipelineVtxInputStateCrtInfo.pVertexBindingDescriptions      = &VtxInputBindDesc;
    PipelineVtxInputStateCrtInfo.vertexAttributeDescriptionCount = static_cast< uint32_t >( std::size( VtxAttrs ) );
    PipelineVtxInputStateCrtInfo.pVertexAttributeDescriptions    = std::data( VtxAttrs );

    auto PipelineVtxInputAssemStateCrtInfo     = VkPipelineInputAssemblyStateCreateInfo();
    PipelineVtxInputAssemStateCrtInfo.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    PipelineVtxInputAssemStateCrtInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PipelineVtxInputAssemStateCrtInfo.primitiveRestartEnable = VK_FALSE;

    auto Viewport     = VkViewport();
    Viewport.x        = 0.0F;
    Viewport.y        = 0.0F;
    Viewport.width    = static_cast< float >( Ctx.SwapchainExtent.width );
    Viewport.height   = static_cast< float >( Ctx.SwapchainExtent.height );
    Viewport.minDepth = 0.0F;
    Viewport.maxDepth = 1.0F;

    auto Scissor     = VkRect2D();
    Scissor.offset.x = 0;
    Scissor.offset.y = 0;
    Scissor.extent   = Ctx.SwapchainExtent;

    auto PipelineViewportStateCrtInfo          = VkPipelineViewportStateCreateInfo();
    PipelineViewportStateCrtInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    PipelineViewportStateCrtInfo.viewportCount = 1;
    PipelineViewportStateCrtInfo.pViewports    = &Viewport;
    PipelineViewportStateCrtInfo.scissorCount  = 1;
    PipelineViewportStateCrtInfo.pScissors     = &Scissor;

    auto PipelineRastStateCrtInfo                    = VkPipelineRasterizationStateCreateInfo();
    PipelineRastStateCrtInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    PipelineRastStateCrtInfo.depthClampEnable        = VK_FALSE;
    PipelineRastStateCrtInfo.rasterizerDiscardEnable = VK_FALSE;
    PipelineRastStateCrtInfo.polygonMode             = VK_POLYGON_MODE_FILL;
    PipelineRastStateCrtInfo.lineWidth               = 1.0F;
    PipelineRastStateCrtInfo.cullMode                = VK_CULL_MODE_BACK_BIT;
    PipelineRastStateCrtInfo.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    PipelineRastStateCrtInfo.depthBiasEnable         = VK_FALSE;
    PipelineRastStateCrtInfo.depthBiasConstantFactor = 0.0F;
    PipelineRastStateCrtInfo.depthBiasClamp          = 0.0F;
    PipelineRastStateCrtInfo.depthBiasSlopeFactor    = 0.0F;

    auto PipelineMultSampleStateCrtInfo                  = VkPipelineMultisampleStateCreateInfo();
    PipelineMultSampleStateCrtInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    PipelineMultSampleStateCrtInfo.sampleShadingEnable   = VK_FALSE;
    PipelineMultSampleStateCrtInfo.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    PipelineMultSampleStateCrtInfo.minSampleShading      = 1.0F;
    PipelineMultSampleStateCrtInfo.pSampleMask           = nullptr;
    PipelineMultSampleStateCrtInfo.alphaToCoverageEnable = VK_FALSE;
    PipelineMultSampleStateCrtInfo.alphaToOneEnable      = VK_FALSE;

    auto PipelineColorBlendAttachState = VkPipelineColorBlendAttachmentState();
    PipelineColorBlendAttachState.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    PipelineColorBlendAttachState.blendEnable         = VK_FALSE;
    PipelineColorBlendAttachState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    PipelineColorBlendAttachState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    PipelineColorBlendAttachState.colorBlendOp        = VK_BLEND_OP_ADD;
    PipelineColorBlendAttachState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    PipelineColorBlendAttachState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    PipelineColorBlendAttachState.alphaBlendOp        = VK_BLEND_OP_ADD;

    auto PipelineColorBlendCrtInfo                = VkPipelineColorBlendStateCreateInfo();
    PipelineColorBlendCrtInfo.sType               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    PipelineColorBlendCrtInfo.logicOpEnable       = VK_FALSE;
    PipelineColorBlendCrtInfo.logicOp             = VK_LOGIC_OP_COPY;
    PipelineColorBlendCrtInfo.attachmentCount     = 1;
    PipelineColorBlendCrtInfo.pAttachments        = &PipelineColorBlendAttachState;
    PipelineColorBlendCrtInfo.blendConstants[ 0 ] = 0.0F;
    PipelineColorBlendCrtInfo.blendConstants[ 1 ] = 0.0F;
    PipelineColorBlendCrtInfo.blendConstants[ 2 ] = 0.0F;
    PipelineColorBlendCrtInfo.blendConstants[ 3 ] = 0.0F;

    auto PipelineDepthStencilStateCrtInfo                  = VkPipelineDepthStencilStateCreateInfo();
    PipelineDepthStencilStateCrtInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    PipelineDepthStencilStateCrtInfo.depthTestEnable       = VK_TRUE;
    PipelineDepthStencilStateCrtInfo.depthWriteEnable      = VK_TRUE;
    PipelineDepthStencilStateCrtInfo.depthCompareOp        = VK_COMPARE_OP_LESS;
    PipelineDepthStencilStateCrtInfo.depthBoundsTestEnable = VK_FALSE;
    PipelineDepthStencilStateCrtInfo.minDepthBounds        = 0.0F;
    PipelineDepthStencilStateCrtInfo.maxDepthBounds        = 1.0F;
    PipelineDepthStencilStateCrtInfo.stencilTestEnable     = VK_FALSE;

    auto VtxPipelineShaderStageCrtInfo   = VkPipelineShaderStageCreateInfo();
    VtxPipelineShaderStageCrtInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    VtxPipelineShaderStageCrtInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    VtxPipelineShaderStageCrtInfo.module = Ctx.VtxShader;
    VtxPipelineShaderStageCrtInfo.pName  = "main";

    auto FragPipelineShaderStageCrtInfo   = VkPipelineShaderStageCreateInfo();
    FragPipelineShaderStageCrtInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    FragPipelineShaderStageCrtInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    FragPipelineShaderStageCrtInfo.module = Ctx.FragShader;
    FragPipelineShaderStageCrtInfo.pName  = "main";

    auto const ShaderStages = std::array{ VtxPipelineShaderStageCrtInfo, FragPipelineShaderStageCrtInfo };

    auto PipelineCrtInfo                = VkGraphicsPipelineCreateInfo();
    PipelineCrtInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineCrtInfo.stageCount          = static_cast< uint32_t >( std::size( ShaderStages ) );
    PipelineCrtInfo.pStages             = std::data( ShaderStages );
    PipelineCrtInfo.pVertexInputState   = &PipelineVtxInputStateCrtInfo;
    PipelineCrtInfo.pInputAssemblyState = &PipelineVtxInputAssemStateCrtInfo;
    PipelineCrtInfo.pViewportState      = &PipelineViewportStateCrtInfo;
    PipelineCrtInfo.pRasterizationState = &PipelineRastStateCrtInfo;
    PipelineCrtInfo.pMultisampleState   = &PipelineMultSampleStateCrtInfo;
    PipelineCrtInfo.pDepthStencilState  = &PipelineDepthStencilStateCrtInfo;
    PipelineCrtInfo.pColorBlendState    = &PipelineColorBlendCrtInfo;
    PipelineCrtInfo.pDynamicState       = nullptr;
    PipelineCrtInfo.layout              = Ctx.PipelineLay;
    PipelineCrtInfo.renderPass          = Ctx.RdrPass;
    PipelineCrtInfo.subpass             = 0;
    PipelineCrtInfo.basePipelineHandle  = nullptr;
    PipelineCrtInfo.basePipelineIndex   = -1;

    [[maybe_unused]] auto Result =
      vkCreateGraphicsPipelines( Ctx.Dev, VK_NULL_HANDLE, 1, &PipelineCrtInfo, nullptr, &Ctx.Pipeline );
    MVK_VERIFY( Result == VK_SUCCESS );
  }

  void destroyPipelines( Context & Ctx ) noexcept
  {
    vkDestroyPipeline( Ctx.Dev, Ctx.Pipeline, nullptr );
  }

  void initSync( Context & Ctx ) noexcept
  {
    auto SemaphoreCrtInfo  = VkSemaphoreCreateInfo();
    SemaphoreCrtInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    auto FenceCrtInfo  = VkFenceCreateInfo();
    FenceCrtInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    FenceCrtInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for ( auto i = size_t( 0 ); i < Context::MaxFramesInFlight; ++i )
    {
      auto Result = vkCreateSemaphore( Ctx.Dev, &SemaphoreCrtInfo, nullptr, &Ctx.ImgAvailableSemaphores[ i ] );
      MVK_VERIFY( Result == VK_SUCCESS );

      Result = vkCreateSemaphore( Ctx.Dev, &SemaphoreCrtInfo, nullptr, &Ctx.RdrFinishedSemaphores[ i ] );
      MVK_VERIFY( Result == VK_SUCCESS );

      Result = vkCreateFence( Ctx.Dev, &FenceCrtInfo, nullptr, &Ctx.FrameInFlightFences[ i ] );
      MVK_VERIFY( Result == VK_SUCCESS );
    }

    Ctx.ImgInFlightFences.resize( Ctx.SwapchainImgCnt, nullptr );
  }

  void destroySync( Context & Ctx ) noexcept
  {
    for ( auto i = size_t( 0 ); i < Context::MaxFramesInFlight; ++i )
    {
      vkDestroySemaphore( Ctx.Dev, Ctx.ImgAvailableSemaphores[ i ], nullptr );
      vkDestroySemaphore( Ctx.Dev, Ctx.RdrFinishedSemaphores[ i ], nullptr );
      vkDestroyFence( Ctx.Dev, Ctx.FrameInFlightFences[ i ], nullptr );
    }
  }

}  // namespace mvk::engine
