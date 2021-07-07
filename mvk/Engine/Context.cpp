#include "Engine/Context.hpp"

#include "Detail/Helpers.hpp"
#include "Detail/Misc.hpp"
#include "Detail/Readers.hpp"
#include "Engine/Debug.hpp"
#include "Engine/Tex.hpp"

namespace Mvk::Engine {

[[nodiscard]] VkExtent2D Context::getFramebufferSize() const noexcept {
  auto Width = 0;
  auto Height = 0;

  do {
    glfwGetFramebufferSize(Window, &Width, &Height);
    glfwWaitEvents();
  } while (Width == 0 || Height == 0);

  return {static_cast<uint32_t>(Width), static_cast<uint32_t>(Height)};
}

Context::Context(std::string const &Name, VkExtent2D Extent) noexcept {

  initWindow(Name, Extent);
  initInstace(Name);
  initDbgMsngr();
  initSurface();
  selectPhysicalDevice();
  selectSurfaceFmt();
  initDevice();
  initLayouts();
  initPools();

  auto CmdBuffAllocInfo = VkCommandBufferAllocateInfo();
  CmdBuffAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  CmdBuffAllocInfo.commandPool = CmdPool;
  CmdBuffAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  CmdBuffAllocInfo.commandBufferCount = 1;

  auto Result =
      vkAllocateCommandBuffers(Device, &CmdBuffAllocInfo, &CurrentCmdBuff);
  MVK_VERIFY(Result == VK_SUCCESS);

  auto CmdBuffBeginInfo = VkCommandBufferBeginInfo();
  CmdBuffBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  CmdBuffBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  CmdBuffBeginInfo.flags = 0;
  CmdBuffBeginInfo.pInheritanceInfo = nullptr;

  vkBeginCommandBuffer(CurrentCmdBuff, &CmdBuffBeginInfo);

  initSwapchain();
  initDepthImg();
  initRenderPass();
  initFramebuffers();
  initSamplers();
  initShaders();
  initPipelines();
  initCmdBuffs();
  initSync();

  vkEndCommandBuffer(CurrentCmdBuff);

  auto SubmitInfo = VkSubmitInfo();
  SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  SubmitInfo.commandBufferCount = 1;
  SubmitInfo.pCommandBuffers = &CurrentCmdBuff;

  vkQueueSubmit(GfxQueue, 1, &SubmitInfo, nullptr);
  vkQueueWaitIdle(GfxQueue);

  vkFreeCommandBuffers(Device, CmdPool, 1, &CurrentCmdBuff);

  CurrentCmdBuff = CmdBuffs[CurrentBuffIdx];
}

Context::~Context() noexcept {
  dstrGarbageSets();
  dstrGarbageMems();
  dstrGarbageBuffs();
  dstrSync();
  dstrPipelines();
  dstrShaders();
  dstrCmdBuffs();
  dstrSamplers();
  dstrFramebuffers();
  dstrRenderPass();
  dstrDepthImg();
  dstrSwapchain();
  dstrPools();
  dstrLayouts();
  dstrDevice();
  dstrSurface();
  dstrDbgMsngr();
  dstrInstance();
  dstrWindow();
}

void Context::initWindow(std::string const &Name, VkExtent2D Extent) noexcept {

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  auto const Callback = [](GLFWwindow *const CallbackWindow,
                           [[maybe_unused]] int const CbWidth,
                           [[maybe_unused]] int const CbHeight) {
    auto const User = glfwGetWindowUserPointer(CallbackWindow);
    auto const CurrentCtx = reinterpret_cast<Context *>(User);
    CurrentCtx->FramebufferResized = true;
  };

  Window = glfwCreateWindow(static_cast<int>(Extent.width),
                            static_cast<int>(Extent.height), Name.c_str(),
                            nullptr, nullptr);

  glfwSetWindowUserPointer(Window, this);
  glfwSetFramebufferSizeCallback(Window, Callback);
}

void Context::initInstace(std::string const &Name) noexcept {
  if constexpr (Context::UseValidation) {
    auto ValidationLayersPropCount = uint32_t(0);
    vkEnumerateInstanceLayerProperties(&ValidationLayersPropCount, nullptr);

    auto ValLayerProps =
        std::vector<VkLayerProperties>(ValidationLayersPropCount);
    vkEnumerateInstanceLayerProperties(&ValidationLayersPropCount,
                                       std::data(ValLayerProps));

    auto const Found = [&ValLayerProps] {
      for (auto const &ValLayerProp : ValLayerProps) {
        for (auto const ValLayer : Context::ValidationLayers) {
          if (std::strcmp(ValLayer, ValLayerProp.layerName) == 0) {
            return true;
          }
        }
      }

      return false;
    }();

    MVK_VERIFY(Found);
  }

  auto AppInfo = VkApplicationInfo();
  AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  AppInfo.pApplicationName = Name.c_str();
  AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  AppInfo.pEngineName = "No Engine";
  AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

  auto ReqInstExtCount = uint32_t(0);
  auto const ReqInstExtData =
      glfwGetRequiredInstanceExtensions(&ReqInstExtCount);

  auto ReqExts = std::vector<char const *>(
      ReqInstExtData, std::next(ReqInstExtData, ReqInstExtCount));

  if constexpr (Context::UseValidation) {
    ReqExts.insert(std::begin(ReqExts),
                   std::begin(Context::ValidationInstanceExtensionss),
                   std::end(Context::ValidationInstanceExtensionss));
  }

  auto InstCrtInfo = VkInstanceCreateInfo();
  InstCrtInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

  if constexpr (Context::UseValidation) {
    InstCrtInfo.pNext = &DbgCrtInfo;
  } else {
    InstCrtInfo.pNext = nullptr;
  }

  InstCrtInfo.pApplicationInfo = &AppInfo;

  if constexpr (Context::UseValidation) {
    InstCrtInfo.enabledLayerCount =
        static_cast<uint32_t>(std::size(Context::ValidationLayers));
    InstCrtInfo.ppEnabledLayerNames = std::data(Context::ValidationLayers);
  } else {
    InstCrtInfo.enabledLayerCount = 0;
    InstCrtInfo.ppEnabledExtensionNames = nullptr;
  }

  InstCrtInfo.enabledExtensionCount = static_cast<uint32_t>(std::size(ReqExts));
  InstCrtInfo.ppEnabledExtensionNames = std::data(ReqExts);

  auto Result = vkCreateInstance(&InstCrtInfo, nullptr, &Instance);
  MVK_VERIFY(Result == VK_SUCCESS);
}

void Context::initDbgMsngr() noexcept {

  if constexpr (Context::UseValidation) {
    auto const CrtDbgUtilMsngr =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT"));

    MVK_VERIFY(CrtDbgUtilMsngr);

    CrtDbgUtilMsngr(Instance, &DbgCrtInfo, nullptr, &DbgMsngr);
  }
}

void Context::initSurface() noexcept {
  glfwCreateWindowSurface(Instance, Window, nullptr, &Surface);
}

void Context::selectPhysicalDevice() noexcept {
  auto AvailablePhysicalDeviceCount = uint32_t(0);
  vkEnumeratePhysicalDevices(Instance, &AvailablePhysicalDeviceCount, nullptr);

  auto AvailablePhysicalDevices =
      std::vector<VkPhysicalDevice>(AvailablePhysicalDeviceCount);
  vkEnumeratePhysicalDevices(Instance, &AvailablePhysicalDeviceCount,
                             std::data(AvailablePhysicalDevices));

  for (auto const AvailablePhysicalDevice : AvailablePhysicalDevices) {
    auto features = VkPhysicalDeviceFeatures();
    vkGetPhysicalDeviceFeatures(AvailablePhysicalDevice, &features);

    if (Detail::chkExtSup(AvailablePhysicalDevice, Context::DeviceExtensions) &&
        Detail::chkFmtAndPresentModeAvailablity(AvailablePhysicalDevice,
                                                Surface) &&
        Detail::queryFamiliyIdxs(AvailablePhysicalDevice, Surface)
            .has_value() &&
        features.samplerAnisotropy) {
      PhysicalDevice = AvailablePhysicalDevice;
      return;
    }
  }

  MVK_VERIFY_NOT_REACHED();
}

void Context::selectSurfaceFmt() noexcept {
  auto FmtCount = uint32_t(0);
  vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &FmtCount,
                                       nullptr);

  auto Fmts = std::vector<VkSurfaceFormatKHR>(FmtCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &FmtCount,
                                       std::data(Fmts));

  for (auto const Fmt : Fmts) {
    if (Fmt.format == VK_FORMAT_B8G8R8A8_SRGB &&
        Fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      SurfaceFmt = Fmt;
      return;
    }
  }
}

void Context::initDevice() noexcept {
  auto const OptQueueIdx = Detail::queryFamiliyIdxs(PhysicalDevice, Surface);

  MVK_VERIFY(OptQueueIdx.has_value());

  auto const QueueIdxs = OptQueueIdx.value();
  GfxQueueIdx = QueueIdxs.first;
  PresentQueueIdx = QueueIdxs.second;

  auto Features = VkPhysicalDeviceFeatures();
  vkGetPhysicalDeviceFeatures(PhysicalDevice, &Features);

  auto const QueuePrio = 1.0F;

  auto GfxQueueCrtInfo = VkDeviceQueueCreateInfo();
  GfxQueueCrtInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  GfxQueueCrtInfo.queueFamilyIndex = GfxQueueIdx;
  GfxQueueCrtInfo.queueCount = 1;
  GfxQueueCrtInfo.pQueuePriorities = &QueuePrio;

  auto PresentQueueCrtInfo = VkDeviceQueueCreateInfo();
  PresentQueueCrtInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  PresentQueueCrtInfo.queueFamilyIndex = PresentQueueIdx;
  PresentQueueCrtInfo.queueCount = 1;
  PresentQueueCrtInfo.pQueuePriorities = &QueuePrio;

  auto const queue_create_info =
      std::array{GfxQueueCrtInfo, PresentQueueCrtInfo};
  auto const queue_create_info_count =
      static_cast<uint32_t>(QueueIdxs.first != QueueIdxs.second ? 2 : 1);

  auto DeviceCrtInfo = VkDeviceCreateInfo();
  DeviceCrtInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  DeviceCrtInfo.queueCreateInfoCount = queue_create_info_count;
  DeviceCrtInfo.pQueueCreateInfos = std::data(queue_create_info);
  DeviceCrtInfo.pEnabledFeatures = &Features;
  DeviceCrtInfo.enabledExtensionCount =
      static_cast<uint32_t>(std::size(Context::DeviceExtensions));
  DeviceCrtInfo.ppEnabledExtensionNames = std::data(Context::DeviceExtensions);

  if constexpr (Context::UseValidation) {
    DeviceCrtInfo.enabledLayerCount =
        static_cast<uint32_t>(std::size(Context::ValidationLayers));
    DeviceCrtInfo.ppEnabledLayerNames = std::data(Context::ValidationLayers);
  } else {
    DeviceCrtInfo.enabledLayerCount = 0;
    DeviceCrtInfo.ppEnabledLayerNames = nullptr;
  }

  auto Result =
      vkCreateDevice(PhysicalDevice, &DeviceCrtInfo, nullptr, &Device);
  MVK_VERIFY(Result == VK_SUCCESS);

  vkGetDeviceQueue(Device, GfxQueueIdx, 0, &GfxQueue);
  vkGetDeviceQueue(Device, PresentQueueIdx, 0, &PresentQueue);
}

void Context::initLayouts() noexcept {
  auto UniformDescriptorSetLayBind = VkDescriptorSetLayoutBinding();
  UniformDescriptorSetLayBind.binding = 0;
  UniformDescriptorSetLayBind.descriptorType =
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  UniformDescriptorSetLayBind.descriptorCount = 1;
  UniformDescriptorSetLayBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  UniformDescriptorSetLayBind.pImmutableSamplers = nullptr;

  auto UniformDescriptorSetLayoutCrtInfo = VkDescriptorSetLayoutCreateInfo();
  UniformDescriptorSetLayoutCrtInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  UniformDescriptorSetLayoutCrtInfo.bindingCount = 1;
  UniformDescriptorSetLayoutCrtInfo.pBindings = &UniformDescriptorSetLayBind;

  auto Result = vkCreateDescriptorSetLayout(
      Device, &UniformDescriptorSetLayoutCrtInfo, nullptr, &UboDescSetLayout);

  MVK_VERIFY(Result == VK_SUCCESS);

  auto SamplerDescriptorSetLayBind = VkDescriptorSetLayoutBinding();
  SamplerDescriptorSetLayBind.binding = 0;
  SamplerDescriptorSetLayBind.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  SamplerDescriptorSetLayBind.descriptorCount = 1;
  SamplerDescriptorSetLayBind.pImmutableSamplers = nullptr;
  SamplerDescriptorSetLayBind.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  auto SamplerDescriptorSetLayCrtInfo = VkDescriptorSetLayoutCreateInfo();
  SamplerDescriptorSetLayCrtInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  SamplerDescriptorSetLayCrtInfo.bindingCount = 1;
  SamplerDescriptorSetLayCrtInfo.pBindings = &SamplerDescriptorSetLayBind;

  Result = vkCreateDescriptorSetLayout(Device, &SamplerDescriptorSetLayCrtInfo,
                                       nullptr, &TexDescSetLayout);

  MVK_VERIFY(Result == VK_SUCCESS);

  auto DescriptorSetLays = std::array{UboDescSetLayout, TexDescSetLayout};

  auto PipelineLayCrtInfo = VkPipelineLayoutCreateInfo();
  PipelineLayCrtInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  PipelineLayCrtInfo.setLayoutCount =
      static_cast<uint32_t>(std::size(DescriptorSetLays));
  PipelineLayCrtInfo.pSetLayouts = std::data(DescriptorSetLays);
  PipelineLayCrtInfo.pushConstantRangeCount = 0;
  PipelineLayCrtInfo.pPushConstantRanges = nullptr;

  Result = vkCreatePipelineLayout(Device, &PipelineLayCrtInfo, nullptr,
                                  &MainPipelineLayout);

  MVK_VERIFY(Result == VK_SUCCESS);
}

void Context::initPools() noexcept {
  auto CmdPoolCrtInfo = VkCommandPoolCreateInfo();
  CmdPoolCrtInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  CmdPoolCrtInfo.queueFamilyIndex = GfxQueueIdx;
  CmdPoolCrtInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  auto Result = vkCreateCommandPool(Device, &CmdPoolCrtInfo, nullptr, &CmdPool);
  MVK_VERIFY(Result == VK_SUCCESS);

  auto UniformDescriptorPoolSize = VkDescriptorPoolSize();
  UniformDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  UniformDescriptorPoolSize.descriptorCount = 32;

  auto SamplerDescriptorPoolSize = VkDescriptorPoolSize();
  SamplerDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  SamplerDescriptorPoolSize.descriptorCount = 32;

  auto const DescriptorPoolSizes =
      std::array{UniformDescriptorPoolSize, SamplerDescriptorPoolSize};

  auto DescriptorPoolCrtInfo = VkDescriptorPoolCreateInfo();
  DescriptorPoolCrtInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  DescriptorPoolCrtInfo.poolSizeCount =
      static_cast<uint32_t>(std::size(DescriptorPoolSizes));
  DescriptorPoolCrtInfo.pPoolSizes = std::data(DescriptorPoolSizes);
  DescriptorPoolCrtInfo.maxSets = 128;
  DescriptorPoolCrtInfo.flags =
      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

  vkCreateDescriptorPool(Device, &DescriptorPoolCrtInfo, nullptr, &DescPool);
}

void Context::initSwapchain() noexcept {
  auto const FamilyIdxs = std::array{GfxQueueIdx, PresentQueueIdx};

  VkSurfaceCapabilitiesKHR Capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface,
                                            &Capabilities);

  auto FramebufferSize = getFramebufferSize();

  auto const present_mode = Detail::choosePresentMode(PhysicalDevice, Surface);
  SwapchainExtent = Detail::chooseExtent(Capabilities, FramebufferSize);
  auto const image_count = Detail::chooseImgCount(Capabilities);

  auto SwapchainCrtInfo = VkSwapchainCreateInfoKHR();
  SwapchainCrtInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  SwapchainCrtInfo.surface = Surface;
  SwapchainCrtInfo.minImageCount = image_count;
  SwapchainCrtInfo.imageFormat = SurfaceFmt.format;
  SwapchainCrtInfo.imageColorSpace = SurfaceFmt.colorSpace;
  SwapchainCrtInfo.imageExtent = SwapchainExtent;
  SwapchainCrtInfo.imageArrayLayers = 1;
  SwapchainCrtInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  SwapchainCrtInfo.preTransform = Capabilities.currentTransform;
  SwapchainCrtInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  SwapchainCrtInfo.presentMode = present_mode;
  SwapchainCrtInfo.clipped = VK_TRUE;
  SwapchainCrtInfo.oldSwapchain = nullptr;

  if (FamilyIdxs[0] != FamilyIdxs[1]) {
    SwapchainCrtInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    SwapchainCrtInfo.queueFamilyIndexCount = 2;
    SwapchainCrtInfo.pQueueFamilyIndices = std::data(FamilyIdxs);
  } else {
    SwapchainCrtInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    SwapchainCrtInfo.queueFamilyIndexCount = 0;
    SwapchainCrtInfo.pQueueFamilyIndices = nullptr;
  }

  auto Result =
      vkCreateSwapchainKHR(Device, &SwapchainCrtInfo, nullptr, &Swapchain);
  MVK_VERIFY(Result == VK_SUCCESS);

  Result =
      vkGetSwapchainImagesKHR(Device, Swapchain, &SwapchainImgCount, nullptr);
  MVK_VERIFY(Result == VK_SUCCESS);

  auto SwapchainImgs = std::vector<VkImage>(SwapchainImgCount);
  vkGetSwapchainImagesKHR(Device, Swapchain, &SwapchainImgCount,
                          std::data(SwapchainImgs));

  SwapchainImgViews.reserve(SwapchainImgCount);

  for (auto const image : SwapchainImgs) {
    auto SwapchainImgViewCrtInfo = VkImageViewCreateInfo();
    SwapchainImgViewCrtInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    SwapchainImgViewCrtInfo.image = image;
    SwapchainImgViewCrtInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    SwapchainImgViewCrtInfo.format = SwapchainCrtInfo.imageFormat;
    SwapchainImgViewCrtInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    SwapchainImgViewCrtInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    SwapchainImgViewCrtInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    SwapchainImgViewCrtInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    SwapchainImgViewCrtInfo.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_COLOR_BIT;
    SwapchainImgViewCrtInfo.subresourceRange.baseMipLevel = 0;
    SwapchainImgViewCrtInfo.subresourceRange.levelCount = 1;
    SwapchainImgViewCrtInfo.subresourceRange.baseArrayLayer = 0;
    SwapchainImgViewCrtInfo.subresourceRange.layerCount = 1;

    auto ImgView = VkImageView();
    Result =
        vkCreateImageView(Device, &SwapchainImgViewCrtInfo, nullptr, &ImgView);
    SwapchainImgViews.push_back(ImgView);
  }
}

void Context::initDepthImg() noexcept {
  auto DepthImgCreateInfo = VkImageCreateInfo();
  DepthImgCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  DepthImgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  DepthImgCreateInfo.extent.width = SwapchainExtent.width;
  DepthImgCreateInfo.extent.height = SwapchainExtent.height;
  DepthImgCreateInfo.extent.depth = 1;
  DepthImgCreateInfo.mipLevels = 1;
  DepthImgCreateInfo.arrayLayers = 1;
  DepthImgCreateInfo.format = VK_FORMAT_D32_SFLOAT;
  DepthImgCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  DepthImgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  DepthImgCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  DepthImgCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  DepthImgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  DepthImgCreateInfo.flags = 0;

  auto Result = vkCreateImage(Device, &DepthImgCreateInfo, nullptr, &DepthImg);
  MVK_VERIFY(Result == VK_SUCCESS);

  auto DepthImgReq = VkMemoryRequirements();
  vkGetImageMemoryRequirements(Device, DepthImg, &DepthImgReq);

  auto const MemTypeIdx =
      Detail::queryMemType(PhysicalDevice, DepthImgReq.memoryTypeBits,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  MVK_VERIFY(MemTypeIdx.has_value());

  auto DepthImgMemAllocInfo = VkMemoryAllocateInfo();
  DepthImgMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  DepthImgMemAllocInfo.allocationSize = DepthImgReq.size;
  DepthImgMemAllocInfo.memoryTypeIndex = MemTypeIdx.value();

  Result =
      vkAllocateMemory(Device, &DepthImgMemAllocInfo, nullptr, &DepthImgMem);
  MVK_VERIFY(Result == VK_SUCCESS);

  Result = vkBindImageMemory(Device, DepthImg, DepthImgMem, 0);
  MVK_VERIFY(Result == VK_SUCCESS);

  auto DepthImgViewCrtInfo = VkImageViewCreateInfo();
  DepthImgViewCrtInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  DepthImgViewCrtInfo.image = DepthImg;
  DepthImgViewCrtInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  DepthImgViewCrtInfo.format = VK_FORMAT_D32_SFLOAT;
  DepthImgViewCrtInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  DepthImgViewCrtInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  DepthImgViewCrtInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  DepthImgViewCrtInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  DepthImgViewCrtInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  DepthImgViewCrtInfo.subresourceRange.baseMipLevel = 0;
  DepthImgViewCrtInfo.subresourceRange.levelCount = 1;
  DepthImgViewCrtInfo.subresourceRange.baseArrayLayer = 0;
  DepthImgViewCrtInfo.subresourceRange.layerCount = 1;

  Result =
      vkCreateImageView(Device, &DepthImgViewCrtInfo, nullptr, &DepthImgView);

  MVK_VERIFY(Result == VK_SUCCESS);

  Tex::transitionImgLayout(getCurrentCmdBuff(), DepthImg,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

void Context::initFramebuffers() noexcept {
  for (auto const ImgView : SwapchainImgViews) {
    auto const Attachments = std::array{ImgView, DepthImgView};

    auto FramebufferCrtInfo = VkFramebufferCreateInfo();
    FramebufferCrtInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferCrtInfo.renderPass = RenderPass;
    FramebufferCrtInfo.attachmentCount =
        static_cast<uint32_t>(std::size(Attachments));
    FramebufferCrtInfo.pAttachments = std::data(Attachments);
    FramebufferCrtInfo.width = SwapchainExtent.width;
    FramebufferCrtInfo.height = SwapchainExtent.height;
    FramebufferCrtInfo.layers = 1;

    auto Framebuffer = VkFramebuffer();

    [[maybe_unused]] auto Result =
        vkCreateFramebuffer(Device, &FramebufferCrtInfo, nullptr, &Framebuffer);
    Framebuffers.push_back(Framebuffer);
    MVK_VERIFY(Result == VK_SUCCESS);
  }
}

void Context::initRenderPass() noexcept {
  auto ColorAttachDesc = VkAttachmentDescription();
  ColorAttachDesc.format = SurfaceFmt.format;
  ColorAttachDesc.samples = VK_SAMPLE_COUNT_1_BIT;
  ColorAttachDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  ColorAttachDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  ColorAttachDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  ColorAttachDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  ColorAttachDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  ColorAttachDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  auto DepthAttachDesc = VkAttachmentDescription();
  DepthAttachDesc.format = VK_FORMAT_D32_SFLOAT;
  DepthAttachDesc.samples = VK_SAMPLE_COUNT_1_BIT;
  DepthAttachDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  DepthAttachDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  DepthAttachDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  DepthAttachDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  DepthAttachDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  DepthAttachDesc.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  auto ColorAttachRef = VkAttachmentReference();
  ColorAttachRef.attachment = 0;
  ColorAttachRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  auto DepthAttachRef = VkAttachmentReference();
  DepthAttachRef.attachment = 1;
  DepthAttachRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  auto SubpassDesc = VkSubpassDescription();
  SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  SubpassDesc.colorAttachmentCount = 1;
  SubpassDesc.pColorAttachments = &ColorAttachRef;
  SubpassDesc.pDepthStencilAttachment = &DepthAttachRef;

  auto SubpassDep = VkSubpassDependency();
  SubpassDep.srcSubpass = VK_SUBPASS_EXTERNAL;
  SubpassDep.dstSubpass = 0;
  SubpassDep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  SubpassDep.srcAccessMask = 0;
  SubpassDep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  SubpassDep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  auto const Attachments = std::array{ColorAttachDesc, DepthAttachDesc};

  auto RenderPassCrtInfo = VkRenderPassCreateInfo();
  RenderPassCrtInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  RenderPassCrtInfo.attachmentCount =
      static_cast<uint32_t>(std::size(Attachments));
  RenderPassCrtInfo.pAttachments = std::data(Attachments);
  RenderPassCrtInfo.subpassCount = 1;
  RenderPassCrtInfo.pSubpasses = &SubpassDesc;
  RenderPassCrtInfo.dependencyCount = 1;
  RenderPassCrtInfo.pDependencies = &SubpassDep;

  [[maybe_unused]] auto Result =
      vkCreateRenderPass(Device, &RenderPassCrtInfo, nullptr, &RenderPass);
  MVK_VERIFY(Result == VK_SUCCESS);
}

void Context::initCmdBuffs() noexcept {
  auto CmdBuffAllocInfo = VkCommandBufferAllocateInfo();
  CmdBuffAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  CmdBuffAllocInfo.commandPool = CmdPool;
  CmdBuffAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  CmdBuffAllocInfo.commandBufferCount = Context::DynamicBuffCount;

  auto Result =
      vkAllocateCommandBuffers(Device, &CmdBuffAllocInfo, std::data(CmdBuffs));
  MVK_VERIFY(Result == VK_SUCCESS);
}

void Context::initShaders() noexcept {
  auto const VtxCode = Detail::readFile("../../shaders/vert.spv");

  auto VtxShaderModuleCrtInfo = VkShaderModuleCreateInfo();
  VtxShaderModuleCrtInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  VtxShaderModuleCrtInfo.codeSize = static_cast<uint32_t>(std::size(VtxCode));
  VtxShaderModuleCrtInfo.pCode =
      reinterpret_cast<uint32_t const *>(std::data(VtxCode));

  auto Result = vkCreateShaderModule(Device, &VtxShaderModuleCrtInfo, nullptr,
                                     &VtxShader);
  MVK_VERIFY(Result == VK_SUCCESS);

  auto const FragCode = Detail::readFile("../../shaders/frag.spv");

  auto FragShaderModuleCrtInfo = VkShaderModuleCreateInfo();
  FragShaderModuleCrtInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  FragShaderModuleCrtInfo.codeSize = static_cast<uint32_t>(std::size(FragCode));
  FragShaderModuleCrtInfo.pCode =
      reinterpret_cast<uint32_t const *>(std::data(FragCode));

  Result = vkCreateShaderModule(Device, &FragShaderModuleCrtInfo, nullptr,
                                &FragShader);
  MVK_VERIFY(Result == VK_SUCCESS);
}

void Context::initSamplers() noexcept {
  auto SamplerCrtInfo = VkSamplerCreateInfo();
  SamplerCrtInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  SamplerCrtInfo.magFilter = VK_FILTER_LINEAR;
  SamplerCrtInfo.minFilter = VK_FILTER_LINEAR;
  SamplerCrtInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  SamplerCrtInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  SamplerCrtInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  SamplerCrtInfo.anisotropyEnable = VK_TRUE;
  SamplerCrtInfo.anisotropyEnable = VK_TRUE;
  SamplerCrtInfo.maxAnisotropy = 16;
  SamplerCrtInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  SamplerCrtInfo.unnormalizedCoordinates = VK_FALSE;
  SamplerCrtInfo.compareEnable = VK_FALSE;
  SamplerCrtInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  SamplerCrtInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  SamplerCrtInfo.mipLodBias = 0.0F;
  SamplerCrtInfo.minLod = 0.0F;
  SamplerCrtInfo.maxLod = std::numeric_limits<float>::max();

  auto Result = vkCreateSampler(Device, &SamplerCrtInfo, nullptr, &TexSampler);
  MVK_VERIFY(Result == VK_SUCCESS);
}

void Context::initPipelines() noexcept {
  auto VtxInputBindDesc = VkVertexInputBindingDescription();
  VtxInputBindDesc.binding = 0;
  VtxInputBindDesc.stride = sizeof(vertex);
  VtxInputBindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  auto PosVtxInputAttrDesc = VkVertexInputAttributeDescription();
  PosVtxInputAttrDesc.binding = 0;
  PosVtxInputAttrDesc.location = 0;
  PosVtxInputAttrDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
  PosVtxInputAttrDesc.offset = offsetof(vertex, pos);

  auto ColorVtxInputAttrDesc = VkVertexInputAttributeDescription();
  ColorVtxInputAttrDesc.binding = 0;
  ColorVtxInputAttrDesc.location = 1;
  ColorVtxInputAttrDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
  ColorVtxInputAttrDesc.offset = offsetof(vertex, color);

  auto TexCoordVtxInputAttrDesc = VkVertexInputAttributeDescription();
  TexCoordVtxInputAttrDesc.binding = 0;
  TexCoordVtxInputAttrDesc.location = 2;
  TexCoordVtxInputAttrDesc.format = VK_FORMAT_R32G32_SFLOAT;
  TexCoordVtxInputAttrDesc.offset = offsetof(vertex, texture_coord);

  auto const VtxAttrs = std::array{PosVtxInputAttrDesc, ColorVtxInputAttrDesc,
                                   TexCoordVtxInputAttrDesc};

  auto PipelineVtxInputStateCrtInfo = VkPipelineVertexInputStateCreateInfo();
  PipelineVtxInputStateCrtInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  PipelineVtxInputStateCrtInfo.vertexBindingDescriptionCount = 1;
  PipelineVtxInputStateCrtInfo.pVertexBindingDescriptions = &VtxInputBindDesc;
  PipelineVtxInputStateCrtInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(std::size(VtxAttrs));
  PipelineVtxInputStateCrtInfo.pVertexAttributeDescriptions =
      std::data(VtxAttrs);

  auto PipelineVtxInputAssemStateCrtInfo =
      VkPipelineInputAssemblyStateCreateInfo();
  PipelineVtxInputAssemStateCrtInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  PipelineVtxInputAssemStateCrtInfo.topology =
      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  PipelineVtxInputAssemStateCrtInfo.primitiveRestartEnable = VK_FALSE;

  auto Viewport = VkViewport();
  Viewport.x = 0.0F;
  Viewport.y = 0.0F;
  Viewport.width = static_cast<float>(SwapchainExtent.width);
  Viewport.height = static_cast<float>(SwapchainExtent.height);
  Viewport.minDepth = 0.0F;
  Viewport.maxDepth = 1.0F;

  auto Scissor = VkRect2D();
  Scissor.offset.x = 0;
  Scissor.offset.y = 0;
  Scissor.extent = SwapchainExtent;

  auto PipelineViewportStateCrtInfo = VkPipelineViewportStateCreateInfo();
  PipelineViewportStateCrtInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  PipelineViewportStateCrtInfo.viewportCount = 1;
  PipelineViewportStateCrtInfo.pViewports = &Viewport;
  PipelineViewportStateCrtInfo.scissorCount = 1;
  PipelineViewportStateCrtInfo.pScissors = &Scissor;

  auto PipelineRastStateCrtInfo = VkPipelineRasterizationStateCreateInfo();
  PipelineRastStateCrtInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  PipelineRastStateCrtInfo.depthClampEnable = VK_FALSE;
  PipelineRastStateCrtInfo.rasterizerDiscardEnable = VK_FALSE;
  PipelineRastStateCrtInfo.polygonMode = VK_POLYGON_MODE_FILL;
  PipelineRastStateCrtInfo.lineWidth = 1.0F;
  PipelineRastStateCrtInfo.cullMode = VK_CULL_MODE_BACK_BIT;
  PipelineRastStateCrtInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  PipelineRastStateCrtInfo.depthBiasEnable = VK_FALSE;
  PipelineRastStateCrtInfo.depthBiasConstantFactor = 0.0F;
  PipelineRastStateCrtInfo.depthBiasClamp = 0.0F;
  PipelineRastStateCrtInfo.depthBiasSlopeFactor = 0.0F;

  auto PipelineMultSampleStateCrtInfo = VkPipelineMultisampleStateCreateInfo();
  PipelineMultSampleStateCrtInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  PipelineMultSampleStateCrtInfo.sampleShadingEnable = VK_FALSE;
  PipelineMultSampleStateCrtInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  PipelineMultSampleStateCrtInfo.minSampleShading = 1.0F;
  PipelineMultSampleStateCrtInfo.pSampleMask = nullptr;
  PipelineMultSampleStateCrtInfo.alphaToCoverageEnable = VK_FALSE;
  PipelineMultSampleStateCrtInfo.alphaToOneEnable = VK_FALSE;

  auto PipelineColorBlendAttachState = VkPipelineColorBlendAttachmentState();
  PipelineColorBlendAttachState.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  PipelineColorBlendAttachState.blendEnable = VK_FALSE;
  PipelineColorBlendAttachState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  PipelineColorBlendAttachState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  PipelineColorBlendAttachState.colorBlendOp = VK_BLEND_OP_ADD;
  PipelineColorBlendAttachState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  PipelineColorBlendAttachState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  PipelineColorBlendAttachState.alphaBlendOp = VK_BLEND_OP_ADD;

  auto PipelineColorBlendCrtInfo = VkPipelineColorBlendStateCreateInfo();
  PipelineColorBlendCrtInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  PipelineColorBlendCrtInfo.logicOpEnable = VK_FALSE;
  PipelineColorBlendCrtInfo.logicOp = VK_LOGIC_OP_COPY;
  PipelineColorBlendCrtInfo.attachmentCount = 1;
  PipelineColorBlendCrtInfo.pAttachments = &PipelineColorBlendAttachState;
  PipelineColorBlendCrtInfo.blendConstants[0] = 0.0F;
  PipelineColorBlendCrtInfo.blendConstants[1] = 0.0F;
  PipelineColorBlendCrtInfo.blendConstants[2] = 0.0F;
  PipelineColorBlendCrtInfo.blendConstants[3] = 0.0F;

  auto PipelineDepthStencilStateCrtInfo =
      VkPipelineDepthStencilStateCreateInfo();
  PipelineDepthStencilStateCrtInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  PipelineDepthStencilStateCrtInfo.depthTestEnable = VK_TRUE;
  PipelineDepthStencilStateCrtInfo.depthWriteEnable = VK_TRUE;
  PipelineDepthStencilStateCrtInfo.depthCompareOp = VK_COMPARE_OP_LESS;
  PipelineDepthStencilStateCrtInfo.depthBoundsTestEnable = VK_FALSE;
  PipelineDepthStencilStateCrtInfo.minDepthBounds = 0.0F;
  PipelineDepthStencilStateCrtInfo.maxDepthBounds = 1.0F;
  PipelineDepthStencilStateCrtInfo.stencilTestEnable = VK_FALSE;

  auto VtxPipelineShaderStageCrtInfo = VkPipelineShaderStageCreateInfo();
  VtxPipelineShaderStageCrtInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  VtxPipelineShaderStageCrtInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  VtxPipelineShaderStageCrtInfo.module = VtxShader;
  VtxPipelineShaderStageCrtInfo.pName = "main";

  auto FragPipelineShaderStageCrtInfo = VkPipelineShaderStageCreateInfo();
  FragPipelineShaderStageCrtInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  FragPipelineShaderStageCrtInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  FragPipelineShaderStageCrtInfo.module = FragShader;
  FragPipelineShaderStageCrtInfo.pName = "main";

  auto const ShaderStages =
      std::array{VtxPipelineShaderStageCrtInfo, FragPipelineShaderStageCrtInfo};

  auto PipelineCrtInfo = VkGraphicsPipelineCreateInfo();
  PipelineCrtInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  PipelineCrtInfo.stageCount = static_cast<uint32_t>(std::size(ShaderStages));
  PipelineCrtInfo.pStages = std::data(ShaderStages);
  PipelineCrtInfo.pVertexInputState = &PipelineVtxInputStateCrtInfo;
  PipelineCrtInfo.pInputAssemblyState = &PipelineVtxInputAssemStateCrtInfo;
  PipelineCrtInfo.pViewportState = &PipelineViewportStateCrtInfo;
  PipelineCrtInfo.pRasterizationState = &PipelineRastStateCrtInfo;
  PipelineCrtInfo.pMultisampleState = &PipelineMultSampleStateCrtInfo;
  PipelineCrtInfo.pDepthStencilState = &PipelineDepthStencilStateCrtInfo;
  PipelineCrtInfo.pColorBlendState = &PipelineColorBlendCrtInfo;
  PipelineCrtInfo.pDynamicState = nullptr;
  PipelineCrtInfo.layout = MainPipelineLayout;
  PipelineCrtInfo.renderPass = RenderPass;
  PipelineCrtInfo.subpass = 0;
  PipelineCrtInfo.basePipelineHandle = nullptr;
  PipelineCrtInfo.basePipelineIndex = -1;

  auto Result = vkCreateGraphicsPipelines(
      Device, VK_NULL_HANDLE, 1, &PipelineCrtInfo, nullptr, &MainPipeline);
  MVK_VERIFY(Result == VK_SUCCESS);
}

void Context::initSync() noexcept {
  auto SemaphoreCrtInfo = VkSemaphoreCreateInfo();
  SemaphoreCrtInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  auto FenceCrtInfo = VkFenceCreateInfo();
  FenceCrtInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  FenceCrtInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (auto i = size_t(0); i < Context::MaxFramesInFlight; ++i) {
    auto Result = vkCreateSemaphore(Device, &SemaphoreCrtInfo, nullptr,
                                    &ImgAvailableSemaphores[i]);
    MVK_VERIFY(Result == VK_SUCCESS);

    Result = vkCreateSemaphore(Device, &SemaphoreCrtInfo, nullptr,
                               &RenderFinishedSemaphores[i]);
    MVK_VERIFY(Result == VK_SUCCESS);

    Result =
        vkCreateFence(Device, &FenceCrtInfo, nullptr, &FrameInFlightFences[i]);
    MVK_VERIFY(Result == VK_SUCCESS);
  }

  ImgInFlightFences.resize(SwapchainImgCount, std::nullopt);
}

void Context::dstrWindow() noexcept {
  glfwDestroyWindow(Window);
  glfwTerminate();
}

void Context::dstrInstance() noexcept { vkDestroyInstance(Instance, nullptr); }

void Context::dstrDbgMsngr() noexcept {
  if constexpr (Context::UseValidation) {
    auto const DestroyDbgMsngr =
        reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT"));

    MVK_VERIFY(DestroyDbgMsngr);

    DestroyDbgMsngr(Instance, DbgMsngr, nullptr);
  }
}

void Context::dstrSurface() noexcept {
  vkDestroySurfaceKHR(Instance, Surface, nullptr);
}
void Context::dstrDevice() noexcept { vkDestroyDevice(Device, nullptr); }

void Context::dstrLayouts() noexcept {
  vkDestroyPipelineLayout(Device, MainPipelineLayout, nullptr);
  vkDestroyDescriptorSetLayout(Device, TexDescSetLayout, nullptr);
  vkDestroyDescriptorSetLayout(Device, UboDescSetLayout, nullptr);
}

void Context::dstrPools() noexcept {
  vkDestroyDescriptorPool(Device, DescPool, nullptr);
  vkDestroyCommandPool(Device, CmdPool, nullptr);
}

void Context::dstrSwapchain() noexcept {
  for (auto const SwapchainImgView : SwapchainImgViews) {
    vkDestroyImageView(Device, SwapchainImgView, nullptr);
  }

  vkDestroySwapchainKHR(Device, Swapchain, nullptr);
}

void Context::dstrDepthImg() noexcept {
  vkDestroyImageView(Device, DepthImgView, nullptr);
  vkFreeMemory(Device, DepthImgMem, nullptr);
  vkDestroyImage(Device, DepthImg, nullptr);
}

void Context::dstrFramebuffers() noexcept {
  for (auto const Framebuffer : Framebuffers)
    vkDestroyFramebuffer(Device, Framebuffer, nullptr);
}

void Context::dstrRenderPass() noexcept {
  vkDestroyRenderPass(Device, RenderPass, nullptr);
}
void Context::dstrCmdBuffs() noexcept {
  vkFreeCommandBuffers(Device, CmdPool, DynamicBuffCount, std::data(CmdBuffs));
}

void Context::dstrShaders() noexcept {
  vkDestroyShaderModule(Device, FragShader, nullptr);
  vkDestroyShaderModule(Device, VtxShader, nullptr);
}

void Context::dstrSamplers() noexcept {
  vkDestroySampler(Device, TexSampler, nullptr);
}
void Context::dstrPipelines() noexcept {
  vkDestroyPipeline(Device, MainPipeline, nullptr);
}
void Context::dstrSync() noexcept {
  for (auto i = size_t(0); i < Context::MaxFramesInFlight; ++i) {
    vkDestroySemaphore(Device, ImgAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(Device, RenderFinishedSemaphores[i], nullptr);
    vkDestroyFence(Device, FrameInFlightFences[i], nullptr);
  }
}

void Context::dstrCurrentGarbageBuffs() noexcept {
  auto &CurrentGarbageBuffs = GarbageBuffs[CurrentGarbageIdx];

  for (auto const CurrentGarbageBuff : CurrentGarbageBuffs) {
    vkDestroyBuffer(Device, CurrentGarbageBuff, nullptr);
  }
}

void Context::dstrCurrentGarbageMems() noexcept {
  auto &CurrentGarbageMems = GarbageMems[CurrentGarbageIdx];

  for (auto const CurrentGarbageMem : CurrentGarbageMems) {
    vkFreeMemory(Device, CurrentGarbageMem, nullptr);
  }
}

void Context::dstrCurrentGarbageSets() noexcept {
  auto &CurrentGarbageDescriptorSets = GarbageDescriptorSets[CurrentGarbageIdx];

  for (auto const CurrentGarbageDescriptorSet : CurrentGarbageDescriptorSets)
    vkFreeDescriptorSets(Device, DescPool, 1, &CurrentGarbageDescriptorSet);
}

void Context::dstrGarbageBuffs() noexcept {
  for (auto const &CurrentGarbageBuffs : GarbageBuffs)
    for (auto const CurrentGarbageBuff : CurrentGarbageBuffs)
      vkDestroyBuffer(Device, CurrentGarbageBuff, nullptr);
}
void Context::dstrGarbageMems() noexcept {
  for (auto const &CurrentGarbageMems : GarbageMems)
    for (auto const CurrentGarbageMem : CurrentGarbageMems)
      vkFreeMemory(Device, CurrentGarbageMem, nullptr);
}
void Context::dstrGarbageSets() noexcept {
  for (auto const &CurrentGarbageSets : GarbageDescriptorSets)
    for (auto const CurrentGarbageSet : CurrentGarbageSets)
      vkFreeDescriptorSets(Device, DescPool, 1, &CurrentGarbageSet);
}

void Context::addMemoryToGarbage(VkDeviceMemory Mem) noexcept {
  auto &CurrentGarbageMems = GarbageMems[CurrentGarbageIdx];
  CurrentGarbageMems.push_back(Mem);
}

void Context::addDescriptorSetsToGarbage(
    Utility::Slice<VkDescriptorSet const> Sets) noexcept {
  auto &CurrentGarbageDescriptorSets = GarbageDescriptorSets[CurrentGarbageIdx];
  CurrentGarbageDescriptorSets.insert(std::begin(CurrentGarbageDescriptorSets),
                                      std::begin(Sets), std::end(Sets));
}

void Context::addBuffersToGarbage(
    Utility::Slice<VkBuffer const> Buffs) noexcept {
  auto &CurrentGarbageBuffs = GarbageBuffs[CurrentGarbageIdx];
  CurrentGarbageBuffs.insert(std::begin(CurrentGarbageBuffs), std::begin(Buffs),
                             std::end(Buffs));
}

void Context::recreateAfterFramebufferChange() noexcept {

  auto CmdBuffAllocInfo = VkCommandBufferAllocateInfo();
  CmdBuffAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  CmdBuffAllocInfo.commandPool = CmdPool;
  CmdBuffAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  CmdBuffAllocInfo.commandBufferCount = 1;

  auto Result =
      vkAllocateCommandBuffers(Device, &CmdBuffAllocInfo, &CurrentCmdBuff);
  MVK_VERIFY(Result == VK_SUCCESS);

  auto CmdBuffBeginInfo = VkCommandBufferBeginInfo();
  CmdBuffBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  CmdBuffBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  CmdBuffBeginInfo.flags = 0;
  CmdBuffBeginInfo.pInheritanceInfo = nullptr;

  vkBeginCommandBuffer(CurrentCmdBuff, &CmdBuffBeginInfo);

  dstrSwapchain();
  SwapchainImgViews.clear();
  initSwapchain();

  dstrDepthImg();
  initDepthImg();

  dstrRenderPass();
  initRenderPass();

  dstrFramebuffers();
  Framebuffers.clear();
  initFramebuffers();

  dstrPipelines();
  initPipelines();

  dstrSync();
  ImgInFlightFences.clear();
  initSync();

  vkEndCommandBuffer(CurrentCmdBuff);

  auto SubmitInfo = VkSubmitInfo();
  SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  SubmitInfo.commandBufferCount = 1;
  SubmitInfo.pCommandBuffers = &CurrentCmdBuff;

  vkQueueSubmit(GfxQueue, 1, &SubmitInfo, nullptr);
  vkQueueWaitIdle(GfxQueue);

  vkFreeCommandBuffers(Device, CmdPool, 1, &CurrentCmdBuff);

  CurrentCmdBuff = CmdBuffs[CurrentBuffIdx];
}

[[nodiscard]] Context::Seconds Context::getCurrentTime() const noexcept {
  auto const CurrentTime = std::chrono::high_resolution_clock::now();
  auto const DeltaTime = CurrentTime - StartTime;
  return std::chrono::duration<float, std::chrono::seconds::period>(DeltaTime)
      .count();
}

[[nodiscard]] std::optional<uint32_t>
Context::acquireNextSwapchainImageIdx() const noexcept {
  auto Idx = uint32_t(0);
  auto ImgAvailableSemaphore = getCurrentImgAvailableSemaphore();
  constexpr auto Timeout = std::numeric_limits<uint64_t>::max();
  auto Result = vkAcquireNextImageKHR(Device, Swapchain, Timeout,
                                      ImgAvailableSemaphore, nullptr, &Idx);

  if (Result != VK_ERROR_OUT_OF_DATE_KHR)
    return Idx;

  return std::nullopt;
}

void Context::updateImgIdx(
    [[maybe_unused]] Utility::Badge<Renderer> Badge) noexcept {
  auto NewImgIdxOpt = acquireNextSwapchainImageIdx();

  if (!NewImgIdxOpt.has_value()) {
    recreateAfterFramebufferChange();
    updateImgIdx(Badge);
  }

  CurrentImgIdx = NewImgIdxOpt.value();
}

void Context::updateIdx(
    [[maybe_unused]] Utility::Badge<Renderer> Badge) noexcept {
  CurrentFrameIdx = (CurrentFrameIdx + 1) % MaxFramesInFlight;
  CurrentBuffIdx = (CurrentBuffIdx + 1) % DynamicBuffCount;
  CurrentGarbageIdx = (CurrentGarbageIdx + 1) % GarbageBuffCount;
  CurrentCmdBuff = CmdBuffs[CurrentBuffIdx];
}

#if 0
[[nodiscard]] Seconds getCurrentTime(Context const &Ctx) noexcept {
  auto const CurrentTime = std::chrono::high_resolution_clock::now();
  auto const DeltaTime = CurrentTime - StartTime;
  return std::chrono::duration<float, std::chrono::seconds::period>(DeltaTime).count();
}

void getFramebufferSize(Context const &Ctx, VkExtent2D *size) noexcept {
  *size = VkExtent2D();

  auto Width = 0;
  auto Height = 0;

  do {
    glfwGetFramebufferSize(Window, &Width, &Height);
    glfwWaitEvents();
  } while (Width == 0 || Height == 0);

  size->width = static_cast<uint32_t>(Width);
  size->height = static_cast<uint32_t>(Height);
}

void beginFrame(Context &Ctx) noexcept {
  CurrentCmdBuff = CmdBuffs[CurrentBuffIdx];

  auto CmdBuffBeginInfo = VkCommandBufferBeginInfo();
  CmdBuffBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  CmdBuffBeginInfo.flags = 0;
  CmdBuffBeginInfo.pInheritanceInfo = nullptr;

  vkBeginCommandBuffer(CurrentCmdBuff, &CmdBuffBeginInfo);
}

void endFrame(Context &Ctx) noexcept {
  vkEndCommandBuffer(CurrentCmdBuff);

  // Wait for the Img in flight to end if it is
  auto const *ImgInFlightFence = ImgInFlightFences[CurrentImgIdx];

  if (ImgInFlightFence != nullptr) {
    vkWaitForFences(Device, 1, ImgInFlightFence, VK_TRUE, std::numeric_limits<int64_t>::max());
  }

  ImgInFlightFences[CurrentImgIdx] = &FrameInFlightFences[CurrentFrameIdx];

  // get current semaphores
  auto const ImgAvailableSemaphores = ImgAvailableSemaphores[CurrentFrameIdx];
  auto const RenderFinishedSemaphore = RenderFinishedSemaphores[CurrentFrameIdx];

  auto const WaitSemaphore = std::array{ImgAvailableSemaphores};
  auto const SigSemaphore = std::array{RenderFinishedSemaphore};
  auto const WaitStages = std::array<VkPipelineStageFlags, 1>{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  auto const CmdBuffs = std::array{CurrentCmdBuff};

  auto SubmitInfo = VkSubmitInfo();
  SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  SubmitInfo.waitSemaphoreCount = static_cast<uint32_t>(std::size(WaitSemaphore));
  SubmitInfo.pWaitSemaphores = std::data(WaitSemaphore);
  SubmitInfo.pWaitDstStageMask = std::data(WaitStages);
  SubmitInfo.commandBufferCount = static_cast<uint32_t>(std::size(CmdBuffs));
  SubmitInfo.pCommandBuffers = std::data(CmdBuffs);
  SubmitInfo.signalSemaphoreCount = static_cast<uint32_t>(std::size(SigSemaphore));
  SubmitInfo.pSignalSemaphores = std::data(SigSemaphore);

  vkResetFences(Device, 1, &FrameInFlightFences[CurrentFrameIdx]);
  vkQueueSubmit(GfxQueue, 1, &SubmitInfo, FrameInFlightFences[CurrentFrameIdx]);

  auto const PresentSignalSemaphore = std::array{RenderFinishedSemaphore};
  auto const Swapchains = std::array{Swapchain};
  auto const ImgIdxs = std::array{CurrentImgIdx};

  auto PresentInfo = VkPresentInfoKHR();
  PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  PresentInfo.waitSemaphoreCount = static_cast<uint32_t>(std::size(PresentSignalSemaphore));
  PresentInfo.pWaitSemaphores = std::data(PresentSignalSemaphore);
  PresentInfo.swapchainCount = static_cast<uint32_t>(std::size(Swapchains));
  PresentInfo.pSwapchains = std::data(Swapchains);
  PresentInfo.pImageIndices = std::data(ImgIdxs);
  PresentInfo.pResults = nullptr;

  auto Result = vkQueuePresentKHR(PresentQueue, &PresentInfo);
  vkQueueWaitIdle(PresentQueue);

  auto const change_Swapchain = (Result == VK_ERROR_OUT_OF_DATE_KHR) || (Result == VK_SUBOPTIMAL_KHR);

  if (change_Swapchain || FramebufferResized) {
    FramebufferResized = false;
    recreateAfterSwapchainChange(Ctx);
    return;
  }

  MVK_VERIFY(VK_SUCCESS == Result);

  CurrentFrameIdx = (CurrentFrameIdx + 1) % Context::MaxFramesInFlight;
  nextBuffer(Ctx);
}
#endif

} // namespace Mvk::Engine
