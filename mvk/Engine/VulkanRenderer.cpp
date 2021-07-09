#include "VulkanRenderer.hpp"

#include "Detail/Misc.hpp"
#include "Detail/Readers.hpp"
#include "Engine/Misc.hpp"
#include "Engine/Model.hpp"
#include "Engine/VulkanContext.hpp"

#include <array>
#include <iostream>

namespace Mvk::Engine
{
  VulkanRenderer::VulkanRenderer() noexcept
  {
    VulkanContext::the().initialize( "Stan Loona", { 600, 600 } );

    auto const Device = VulkanContext::the().getDevice();

    initLayouts();
    initPools();

    auto CmdBuffAllocInfo               = VkCommandBufferAllocateInfo();
    CmdBuffAllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CmdBuffAllocInfo.commandPool        = CmdPool;
    CmdBuffAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CmdBuffAllocInfo.commandBufferCount = 1;

    auto Result = vkAllocateCommandBuffers( Device, &CmdBuffAllocInfo, &CurrentCmdBuff );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto CmdBuffBeginInfo             = VkCommandBufferBeginInfo();
    CmdBuffBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CmdBuffBeginInfo.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CmdBuffBeginInfo.flags            = 0;
    CmdBuffBeginInfo.pInheritanceInfo = nullptr;

    vkBeginCommandBuffer( CurrentCmdBuff, &CmdBuffBeginInfo );

    initSwapchain();
    initDepthImg();
    initRenderPass();
    initFramebuffers();
    initShaders();
    initPipelines();
    initCmdBuffs();
    initSync();

    vkEndCommandBuffer( CurrentCmdBuff );

    auto SubmitInfo               = VkSubmitInfo();
    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers    = &CurrentCmdBuff;

    auto const GfxQueue = VulkanContext::the().getGraphicsQueue();

    vkQueueSubmit( GfxQueue, 1, &SubmitInfo, nullptr );
    vkQueueWaitIdle( GfxQueue );

    vkFreeCommandBuffers( Device, CmdPool, 1, &CurrentCmdBuff );

    CurrentCmdBuff = CmdBuffs[CurrentBuffIdx];
  }

  VulkanRenderer::~VulkanRenderer() noexcept
  {
    dstrSync();
    dstrCmdBuffs();
    dstrPipelines();
    dstrShaders();
    dstrFramebuffers();
    dstrRenderPass();
    dstrDepthImg();
    dstrSwapchain();
    dstrPools();
    dstrLayouts();
    Models.clear();
    VulkanContext::the().shutdown();
  }

  void VulkanRenderer::initLayouts() noexcept
  {
    auto const Device = VulkanContext::the().getDevice();

    auto UniformDescriptorSetLayBind               = VkDescriptorSetLayoutBinding();
    UniformDescriptorSetLayBind.binding            = 0;
    UniformDescriptorSetLayBind.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    UniformDescriptorSetLayBind.descriptorCount    = 1;
    UniformDescriptorSetLayBind.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    UniformDescriptorSetLayBind.pImmutableSamplers = nullptr;

    auto SamplerDescriptorSetLayBind               = VkDescriptorSetLayoutBinding();
    SamplerDescriptorSetLayBind.binding            = 1;
    SamplerDescriptorSetLayBind.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    SamplerDescriptorSetLayBind.descriptorCount    = 1;
    SamplerDescriptorSetLayBind.pImmutableSamplers = nullptr;
    SamplerDescriptorSetLayBind.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

    auto const DescriptorBindings = std::array{ UniformDescriptorSetLayBind, SamplerDescriptorSetLayBind };

    auto UniformDescriptorSetLayoutCrtInfo         = VkDescriptorSetLayoutCreateInfo();
    UniformDescriptorSetLayoutCrtInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    UniformDescriptorSetLayoutCrtInfo.bindingCount = static_cast<uint32_t>( std::size( DescriptorBindings ) );
    UniformDescriptorSetLayoutCrtInfo.pBindings    = std::data( DescriptorBindings );

    auto Result = vkCreateDescriptorSetLayout( Device, &UniformDescriptorSetLayoutCrtInfo, nullptr, &UboTexDescSetLayout );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto DescriptorSetLays = std::array{ UboTexDescSetLayout };

    auto PipelineLayCrtInfo                   = VkPipelineLayoutCreateInfo();
    PipelineLayCrtInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayCrtInfo.setLayoutCount         = static_cast<uint32_t>( std::size( DescriptorSetLays ) );
    PipelineLayCrtInfo.pSetLayouts            = std::data( DescriptorSetLays );
    PipelineLayCrtInfo.pushConstantRangeCount = 0;
    PipelineLayCrtInfo.pPushConstantRanges    = nullptr;

    Result = vkCreatePipelineLayout( Device, &PipelineLayCrtInfo, nullptr, &MainPipelineLayout );

    MVK_VERIFY( Result == VK_SUCCESS );
  }

  void VulkanRenderer::initPools() noexcept
  {
    auto const Device            = VulkanContext::the().getDevice();
    auto const GfxQueueFamilyIdx = VulkanContext::the().getGraphicsQueueFamilyIdx();

    auto CmdPoolCrtInfo             = VkCommandPoolCreateInfo();
    CmdPoolCrtInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    CmdPoolCrtInfo.queueFamilyIndex = GfxQueueFamilyIdx;
    CmdPoolCrtInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    auto Result = vkCreateCommandPool( Device, &CmdPoolCrtInfo, nullptr, &CmdPool );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto UniformDescriptorPoolSize            = VkDescriptorPoolSize();
    UniformDescriptorPoolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    UniformDescriptorPoolSize.descriptorCount = 32;

    auto SamplerDescriptorPoolSize            = VkDescriptorPoolSize();
    SamplerDescriptorPoolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    SamplerDescriptorPoolSize.descriptorCount = 32;

    auto const DescriptorPoolSizes = std::array{ UniformDescriptorPoolSize, SamplerDescriptorPoolSize };

    auto DescriptorPoolCrtInfo          = VkDescriptorPoolCreateInfo();
    DescriptorPoolCrtInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    DescriptorPoolCrtInfo.poolSizeCount = static_cast<uint32_t>( std::size( DescriptorPoolSizes ) );
    DescriptorPoolCrtInfo.pPoolSizes    = std::data( DescriptorPoolSizes );
    DescriptorPoolCrtInfo.maxSets       = 128;
    DescriptorPoolCrtInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    vkCreateDescriptorPool( Device, &DescriptorPoolCrtInfo, nullptr, &DescPool );
  }

  void VulkanRenderer::initSwapchain() noexcept
  {
    auto const Device                = VulkanContext::the().getDevice();
    auto const GfxQueueFamilyIdx     = VulkanContext::the().getGraphicsQueueFamilyIdx();
    auto const PresentQueueFamilyIdx = VulkanContext::the().getPresentQueueFamilyIdx();

    auto const FamilyIdxs = std::array{ GfxQueueFamilyIdx, PresentQueueFamilyIdx };

    auto const PhysicalDevice = VulkanContext::the().getPhysicalDevice();
    auto const Surface        = VulkanContext::the().getSurface();

    VkSurfaceCapabilitiesKHR Capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( PhysicalDevice, Surface, &Capabilities );

    auto const FramebufferSize = VulkanContext::the().getFramebufferSize();
    auto const SurfaceFmt      = VulkanContext::the().getSurfaceFmt();

    auto const present_mode = Detail::choosePresentMode( PhysicalDevice, Surface );
    SwapchainExtent         = Detail::chooseExtent( Capabilities, FramebufferSize );
    auto const image_count  = Detail::chooseImgCount( Capabilities );

    auto SwapchainCrtInfo             = VkSwapchainCreateInfoKHR();
    SwapchainCrtInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    SwapchainCrtInfo.surface          = Surface;
    SwapchainCrtInfo.minImageCount    = image_count;
    SwapchainCrtInfo.imageFormat      = SurfaceFmt.format;
    SwapchainCrtInfo.imageColorSpace  = SurfaceFmt.colorSpace;
    SwapchainCrtInfo.imageExtent      = SwapchainExtent;
    SwapchainCrtInfo.imageArrayLayers = 1;
    SwapchainCrtInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    SwapchainCrtInfo.preTransform     = Capabilities.currentTransform;
    SwapchainCrtInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    SwapchainCrtInfo.presentMode      = present_mode;
    SwapchainCrtInfo.clipped          = VK_TRUE;
    SwapchainCrtInfo.oldSwapchain     = nullptr;

    if ( FamilyIdxs[0] != FamilyIdxs[1] )
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

    auto Result = vkCreateSwapchainKHR( Device, &SwapchainCrtInfo, nullptr, &Swapchain );
    MVK_VERIFY( Result == VK_SUCCESS );

    Result = vkGetSwapchainImagesKHR( Device, Swapchain, &SwapchainImgCount, nullptr );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto SwapchainImgs = std::vector<VkImage>( SwapchainImgCount );
    vkGetSwapchainImagesKHR( Device, Swapchain, &SwapchainImgCount, std::data( SwapchainImgs ) );

    SwapchainImgViews.reserve( SwapchainImgCount );

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
      Result       = vkCreateImageView( Device, &SwapchainImgViewCrtInfo, nullptr, &ImgView );
      SwapchainImgViews.push_back( ImgView );
    }
  }

  void VulkanRenderer::initDepthImg() noexcept
  {
    auto DepthImgCreateInfo          = VkImageCreateInfo();
    DepthImgCreateInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    DepthImgCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
    DepthImgCreateInfo.extent.width  = SwapchainExtent.width;
    DepthImgCreateInfo.extent.height = SwapchainExtent.height;
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

    auto const Device         = VulkanContext::the().getDevice();
    auto const PhysicalDevice = VulkanContext::the().getPhysicalDevice();

    auto Result = vkCreateImage( Device, &DepthImgCreateInfo, nullptr, &DepthImg );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto DepthImgReq = VkMemoryRequirements();
    vkGetImageMemoryRequirements( Device, DepthImg, &DepthImgReq );

    auto const MemTypeIdx = Detail::queryMemType( PhysicalDevice, DepthImgReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    MVK_VERIFY( MemTypeIdx.has_value() );

    auto DepthImgMemAllocInfo            = VkMemoryAllocateInfo();
    DepthImgMemAllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    DepthImgMemAllocInfo.allocationSize  = DepthImgReq.size;
    DepthImgMemAllocInfo.memoryTypeIndex = MemTypeIdx.value();
    Result                               = vkAllocateMemory( Device, &DepthImgMemAllocInfo, nullptr, &DepthImgMem );
    MVK_VERIFY( Result == VK_SUCCESS );

    Result = vkBindImageMemory( Device, DepthImg, DepthImgMem, 0 );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto DepthImgViewCrtInfo                            = VkImageViewCreateInfo();
    DepthImgViewCrtInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    DepthImgViewCrtInfo.image                           = DepthImg;
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

    Result = vkCreateImageView( Device, &DepthImgViewCrtInfo, nullptr, &DepthImgView );
    MVK_VERIFY( Result == VK_SUCCESS );

    Detail::transitionImgLayout( CurrentCmdBuff, DepthImg, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1 );
  }

  void VulkanRenderer::initFramebuffers() noexcept
  {
    for ( auto const ImgView : SwapchainImgViews )
    {
      auto const Attachments = std::array{ ImgView, DepthImgView };

      auto FramebufferCrtInfo            = VkFramebufferCreateInfo();
      FramebufferCrtInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      FramebufferCrtInfo.renderPass      = RenderPass;
      FramebufferCrtInfo.attachmentCount = static_cast<uint32_t>( std::size( Attachments ) );
      FramebufferCrtInfo.pAttachments    = std::data( Attachments );
      FramebufferCrtInfo.width           = SwapchainExtent.width;
      FramebufferCrtInfo.height          = SwapchainExtent.height;
      FramebufferCrtInfo.layers          = 1;

      auto Device      = VulkanContext::the().getDevice();
      auto Framebuffer = VkFramebuffer();

      [[maybe_unused]] auto Result = vkCreateFramebuffer( Device, &FramebufferCrtInfo, nullptr, &Framebuffer );
      Framebuffers.push_back( Framebuffer );
      MVK_VERIFY( Result == VK_SUCCESS );
    }
  }

  void VulkanRenderer::initRenderPass() noexcept
  {
    auto const SurfaceFmt = VulkanContext::the().getSurfaceFmt();

    auto ColorAttachDesc           = VkAttachmentDescription();
    ColorAttachDesc.format         = SurfaceFmt.format;
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

    auto RenderPassCrtInfo            = VkRenderPassCreateInfo();
    RenderPassCrtInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassCrtInfo.attachmentCount = static_cast<uint32_t>( std::size( Attachments ) );
    RenderPassCrtInfo.pAttachments    = std::data( Attachments );
    RenderPassCrtInfo.subpassCount    = 1;
    RenderPassCrtInfo.pSubpasses      = &SubpassDesc;
    RenderPassCrtInfo.dependencyCount = 1;
    RenderPassCrtInfo.pDependencies   = &SubpassDep;

    auto const Device = VulkanContext::the().getDevice();

    [[maybe_unused]] auto Result = vkCreateRenderPass( Device, &RenderPassCrtInfo, nullptr, &RenderPass );
    MVK_VERIFY( Result == VK_SUCCESS );
  }

  void VulkanRenderer::initCmdBuffs() noexcept
  {
    auto CmdBuffAllocInfo               = VkCommandBufferAllocateInfo();
    CmdBuffAllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CmdBuffAllocInfo.commandPool        = CmdPool;
    CmdBuffAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CmdBuffAllocInfo.commandBufferCount = DynamicBuffCount;

    auto const Device = VulkanContext::the().getDevice();

    auto Result = vkAllocateCommandBuffers( Device, &CmdBuffAllocInfo, std::data( CmdBuffs ) );
    MVK_VERIFY( Result == VK_SUCCESS );
  }

  void VulkanRenderer::initShaders() noexcept
  {
    auto const VtxCode = Detail::readFile( "../../shaders/vert.spv" );

    auto VtxShaderModuleCrtInfo     = VkShaderModuleCreateInfo();
    VtxShaderModuleCrtInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    VtxShaderModuleCrtInfo.codeSize = static_cast<uint32_t>( std::size( VtxCode ) );
    VtxShaderModuleCrtInfo.pCode    = reinterpret_cast<uint32_t const *>( std::data( VtxCode ) );

    auto const Device = VulkanContext::the().getDevice();

    auto Result = vkCreateShaderModule( Device, &VtxShaderModuleCrtInfo, nullptr, &VtxShader );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto const FragCode = Detail::readFile( "../../shaders/frag.spv" );

    auto FragShaderModuleCrtInfo     = VkShaderModuleCreateInfo();
    FragShaderModuleCrtInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    FragShaderModuleCrtInfo.codeSize = static_cast<uint32_t>( std::size( FragCode ) );
    FragShaderModuleCrtInfo.pCode    = reinterpret_cast<uint32_t const *>( std::data( FragCode ) );

    Result = vkCreateShaderModule( Device, &FragShaderModuleCrtInfo, nullptr, &FragShader );
    MVK_VERIFY( Result == VK_SUCCESS );
  }

  void VulkanRenderer::initPipelines() noexcept
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

    auto PipelineVtxInputStateCrtInfo                            = VkPipelineVertexInputStateCreateInfo();
    PipelineVtxInputStateCrtInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    PipelineVtxInputStateCrtInfo.vertexBindingDescriptionCount   = 1;
    PipelineVtxInputStateCrtInfo.pVertexBindingDescriptions      = &VtxInputBindDesc;
    PipelineVtxInputStateCrtInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>( std::size( VtxAttrs ) );
    PipelineVtxInputStateCrtInfo.pVertexAttributeDescriptions    = std::data( VtxAttrs );

    auto PipelineVtxInputAssemStateCrtInfo                   = VkPipelineInputAssemblyStateCreateInfo();
    PipelineVtxInputAssemStateCrtInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    PipelineVtxInputAssemStateCrtInfo.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PipelineVtxInputAssemStateCrtInfo.primitiveRestartEnable = VK_FALSE;

    auto Viewport     = VkViewport();
    Viewport.x        = 0.0F;
    Viewport.y        = 0.0F;
    Viewport.width    = static_cast<float>( SwapchainExtent.width );
    Viewport.height   = static_cast<float>( SwapchainExtent.height );
    Viewport.minDepth = 0.0F;
    Viewport.maxDepth = 1.0F;

    auto Scissor     = VkRect2D();
    Scissor.offset.x = 0;
    Scissor.offset.y = 0;
    Scissor.extent   = SwapchainExtent;

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

    auto PipelineColorBlendCrtInfo              = VkPipelineColorBlendStateCreateInfo();
    PipelineColorBlendCrtInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    PipelineColorBlendCrtInfo.logicOpEnable     = VK_FALSE;
    PipelineColorBlendCrtInfo.logicOp           = VK_LOGIC_OP_COPY;
    PipelineColorBlendCrtInfo.attachmentCount   = 1;
    PipelineColorBlendCrtInfo.pAttachments      = &PipelineColorBlendAttachState;
    PipelineColorBlendCrtInfo.blendConstants[0] = 0.0F;
    PipelineColorBlendCrtInfo.blendConstants[1] = 0.0F;
    PipelineColorBlendCrtInfo.blendConstants[2] = 0.0F;
    PipelineColorBlendCrtInfo.blendConstants[3] = 0.0F;

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
    VtxPipelineShaderStageCrtInfo.module = VtxShader;
    VtxPipelineShaderStageCrtInfo.pName  = "main";

    auto FragPipelineShaderStageCrtInfo   = VkPipelineShaderStageCreateInfo();
    FragPipelineShaderStageCrtInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    FragPipelineShaderStageCrtInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    FragPipelineShaderStageCrtInfo.module = FragShader;
    FragPipelineShaderStageCrtInfo.pName  = "main";

    auto const ShaderStages = std::array{ VtxPipelineShaderStageCrtInfo, FragPipelineShaderStageCrtInfo };

    auto PipelineCrtInfo                = VkGraphicsPipelineCreateInfo();
    PipelineCrtInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineCrtInfo.stageCount          = static_cast<uint32_t>( std::size( ShaderStages ) );
    PipelineCrtInfo.pStages             = std::data( ShaderStages );
    PipelineCrtInfo.pVertexInputState   = &PipelineVtxInputStateCrtInfo;
    PipelineCrtInfo.pInputAssemblyState = &PipelineVtxInputAssemStateCrtInfo;
    PipelineCrtInfo.pViewportState      = &PipelineViewportStateCrtInfo;
    PipelineCrtInfo.pRasterizationState = &PipelineRastStateCrtInfo;
    PipelineCrtInfo.pMultisampleState   = &PipelineMultSampleStateCrtInfo;
    PipelineCrtInfo.pDepthStencilState  = &PipelineDepthStencilStateCrtInfo;
    PipelineCrtInfo.pColorBlendState    = &PipelineColorBlendCrtInfo;
    PipelineCrtInfo.pDynamicState       = nullptr;
    PipelineCrtInfo.layout              = MainPipelineLayout;
    PipelineCrtInfo.renderPass          = RenderPass;
    PipelineCrtInfo.subpass             = 0;
    PipelineCrtInfo.basePipelineHandle  = nullptr;
    PipelineCrtInfo.basePipelineIndex   = -1;

    auto Device = VulkanContext::the().getDevice();

    auto Result = vkCreateGraphicsPipelines( Device, VK_NULL_HANDLE, 1, &PipelineCrtInfo, nullptr, &MainPipeline );
    MVK_VERIFY( Result == VK_SUCCESS );
  }

  void VulkanRenderer::initSync() noexcept
  {
    auto SemaphoreCrtInfo  = VkSemaphoreCreateInfo();
    SemaphoreCrtInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    auto FenceCrtInfo  = VkFenceCreateInfo();
    FenceCrtInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    FenceCrtInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    auto const Device = VulkanContext::the().getDevice();

    for ( auto i = size_t( 0 ); i < MaxFramesInFlight; ++i )
    {
      auto Result = vkCreateSemaphore( Device, &SemaphoreCrtInfo, nullptr, &ImgAvailableSemaphores[i] );
      MVK_VERIFY( Result == VK_SUCCESS );

      Result = vkCreateSemaphore( Device, &SemaphoreCrtInfo, nullptr, &RenderFinishedSemaphores[i] );
      MVK_VERIFY( Result == VK_SUCCESS );

      Result = vkCreateFence( Device, &FenceCrtInfo, nullptr, &FrameInFlightFences[i] );
      MVK_VERIFY( Result == VK_SUCCESS );
    }

    ImgInFlightFences.resize( SwapchainImgCount, std::nullopt );
  }

  void VulkanRenderer::dstrLayouts() noexcept
  {
    auto const Device = VulkanContext::the().getDevice();
    vkDestroyPipelineLayout( Device, MainPipelineLayout, nullptr );
    vkDestroyDescriptorSetLayout( Device, UboTexDescSetLayout, nullptr );
  }

  void VulkanRenderer::dstrPools() noexcept
  {
    auto const Device = VulkanContext::the().getDevice();

    for ( auto const DescSet : ModelDescSets )
    {
      vkFreeDescriptorSets( Device, DescPool, 1, &DescSet );
    }

    vkDestroyDescriptorPool( Device, DescPool, nullptr );
    vkDestroyCommandPool( Device, CmdPool, nullptr );
  }

  void VulkanRenderer::dstrSwapchain() noexcept
  {
    auto const Device = VulkanContext::the().getDevice();
    for ( auto const SwapchainImgView : SwapchainImgViews )
    {
      vkDestroyImageView( Device, SwapchainImgView, nullptr );
    }

    vkDestroySwapchainKHR( Device, Swapchain, nullptr );
  }

  void VulkanRenderer::dstrDepthImg() noexcept
  {
    auto const Device = VulkanContext::the().getDevice();
    vkDestroyImageView( Device, DepthImgView, nullptr );
    vkFreeMemory( Device, DepthImgMem, nullptr );
    vkDestroyImage( Device, DepthImg, nullptr );
  }

  void VulkanRenderer::dstrFramebuffers() noexcept
  {
    auto const Device = VulkanContext::the().getDevice();
    for ( auto const Framebuffer : Framebuffers )
      vkDestroyFramebuffer( Device, Framebuffer, nullptr );
  }

  void VulkanRenderer::dstrRenderPass() noexcept
  {
    auto const Device = VulkanContext::the().getDevice();
    vkDestroyRenderPass( Device, RenderPass, nullptr );
  }
  void VulkanRenderer::dstrCmdBuffs() noexcept
  {
    auto const Device = VulkanContext::the().getDevice();
    vkFreeCommandBuffers( Device, CmdPool, DynamicBuffCount, std::data( CmdBuffs ) );
  }

  void VulkanRenderer::dstrShaders() noexcept
  {
    auto const Device = VulkanContext::the().getDevice();
    vkDestroyShaderModule( Device, FragShader, nullptr );
    vkDestroyShaderModule( Device, VtxShader, nullptr );
  }

  void VulkanRenderer::dstrPipelines() noexcept
  {
    auto const Device = VulkanContext::the().getDevice();
    vkDestroyPipeline( Device, MainPipeline, nullptr );
  }

  void VulkanRenderer::dstrSync() noexcept
  {
    auto const Device = VulkanContext::the().getDevice();
    for ( auto i = size_t( 0 ); i < MaxFramesInFlight; ++i )
    {
      vkDestroySemaphore( Device, ImgAvailableSemaphores[i], nullptr );
      vkDestroySemaphore( Device, RenderFinishedSemaphores[i], nullptr );
      vkDestroyFence( Device, FrameInFlightFences[i], nullptr );
    }
  }

  void VulkanRenderer::recreateAfterFramebufferChange() noexcept
  {
    auto const Device = VulkanContext::the().getDevice();

    auto CmdBuffAllocInfo               = VkCommandBufferAllocateInfo();
    CmdBuffAllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CmdBuffAllocInfo.commandPool        = CmdPool;
    CmdBuffAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CmdBuffAllocInfo.commandBufferCount = 1;

    auto Result = vkAllocateCommandBuffers( Device, &CmdBuffAllocInfo, &CurrentCmdBuff );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto CmdBuffBeginInfo             = VkCommandBufferBeginInfo();
    CmdBuffBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CmdBuffBeginInfo.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CmdBuffBeginInfo.flags            = 0;
    CmdBuffBeginInfo.pInheritanceInfo = nullptr;

    vkBeginCommandBuffer( CurrentCmdBuff, &CmdBuffBeginInfo );

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

    vkEndCommandBuffer( CurrentCmdBuff );

    auto SubmitInfo               = VkSubmitInfo();
    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers    = &CurrentCmdBuff;

    auto const GfxQueue = VulkanContext::the().getGraphicsQueue();

    vkQueueSubmit( GfxQueue, 1, &SubmitInfo, nullptr );
    vkQueueWaitIdle( GfxQueue );

    vkFreeCommandBuffers( Device, CmdPool, 1, &CurrentCmdBuff );

    CurrentCmdBuff = CmdBuffs[CurrentBuffIdx];
  }

  [[nodiscard]] ModelID VulkanRenderer::loadModel() noexcept
  {
    auto const Device = VulkanContext::the().getDevice();

    // Kind of dumb doing this everytime a commandbuffer is used outside draw
    auto CmdBuffAllocInfo               = VkCommandBufferAllocateInfo();
    CmdBuffAllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CmdBuffAllocInfo.commandPool        = CmdPool;
    CmdBuffAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CmdBuffAllocInfo.commandBufferCount = 1;

    auto Result = vkAllocateCommandBuffers( Device, &CmdBuffAllocInfo, &CurrentCmdBuff );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto CmdBuffBeginInfo             = VkCommandBufferBeginInfo();
    CmdBuffBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CmdBuffBeginInfo.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CmdBuffBeginInfo.flags            = 0;
    CmdBuffBeginInfo.pInheritanceInfo = nullptr;

    vkBeginCommandBuffer( CurrentCmdBuff, &CmdBuffBeginInfo );
    auto [Texture, Width, Height] = Detail::loadTex( "../../assets/viking_room.png" );
    auto [Vtx, Idx]               = Detail::readObj( "../../assets/viking_room.obj" );

    auto const VtxBytes = std::as_bytes( std::span( Vtx ) );
    auto const IdxBytes = std::as_bytes( std::span( Idx ) );

    auto NewModel = std::make_unique<Model>( std::size( VtxBytes ), std::size( IdxBytes ), sizeof( PVM ), Width, Height );
    NewModel->Vbo.map( CurrentCmdBuff, VtxBytes );
    NewModel->Ibo.map( CurrentCmdBuff, Idx );
    NewModel->Tex.transitionLayout( CurrentCmdBuff, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
    NewModel->Tex.map( CurrentCmdBuff, std::as_bytes( std::span( Texture ) ) );
    NewModel->Tex.generateMips( CurrentCmdBuff );

    Models.push_back( std::move( NewModel ) );

    auto SubmitInfo               = VkSubmitInfo();
    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers    = &CurrentCmdBuff;

    auto const GfxQueue = VulkanContext::the().getGraphicsQueue();

    vkEndCommandBuffer( CurrentCmdBuff );
    vkQueueSubmit( GfxQueue, 1, &SubmitInfo, nullptr );
    vkQueueWaitIdle( GfxQueue );

    vkFreeCommandBuffers( Device, CmdPool, 1, &CurrentCmdBuff );

    CurrentCmdBuff = CmdBuffs[CurrentBuffIdx];

    auto DescSetAllocInfo               = VkDescriptorSetAllocateInfo();
    DescSetAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DescSetAllocInfo.descriptorPool     = DescPool;
    DescSetAllocInfo.descriptorSetCount = 1;
    DescSetAllocInfo.pSetLayouts        = &UboTexDescSetLayout;

    auto DescSet = VkDescriptorSet();
    Result       = vkAllocateDescriptorSets( Device, &DescSetAllocInfo, &DescSet );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto UboDescriptorInfo   = VkDescriptorBufferInfo();
    UboDescriptorInfo.buffer = Models.back()->Ubo.getBuffer();
    UboDescriptorInfo.offset = 0;
    UboDescriptorInfo.range  = sizeof( PVM );

    auto ImgDescriptorImgInfo        = VkDescriptorImageInfo();
    ImgDescriptorImgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ImgDescriptorImgInfo.imageView   = Models.back()->Tex.getImgView();
    ImgDescriptorImgInfo.sampler     = Models.back()->Tex.getSampler();

    auto UboWriteDescriptorSet             = VkWriteDescriptorSet();
    UboWriteDescriptorSet.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    UboWriteDescriptorSet.dstSet           = DescSet;
    UboWriteDescriptorSet.dstBinding       = 0;
    UboWriteDescriptorSet.dstArrayElement  = 0;
    UboWriteDescriptorSet.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    UboWriteDescriptorSet.descriptorCount  = 1;
    UboWriteDescriptorSet.pBufferInfo      = &UboDescriptorInfo;
    UboWriteDescriptorSet.pImageInfo       = nullptr;
    UboWriteDescriptorSet.pTexelBufferView = nullptr;

    auto ImgWriteDescriptorSet             = VkWriteDescriptorSet();
    ImgWriteDescriptorSet.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    ImgWriteDescriptorSet.dstSet           = DescSet;
    ImgWriteDescriptorSet.dstBinding       = 1;
    ImgWriteDescriptorSet.dstArrayElement  = 0;
    ImgWriteDescriptorSet.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ImgWriteDescriptorSet.descriptorCount  = 1;
    ImgWriteDescriptorSet.pBufferInfo      = nullptr;
    ImgWriteDescriptorSet.pImageInfo       = &ImgDescriptorImgInfo;
    ImgWriteDescriptorSet.pTexelBufferView = nullptr;

    auto writes = std::array{ UboWriteDescriptorSet, ImgWriteDescriptorSet };

    vkUpdateDescriptorSets( Device, static_cast<uint32_t>( std::size( writes ) ), std::data( writes ), 0, nullptr );

    ModelDescSets.push_back( DescSet );

    return std::size( Models ) - 1;
  }

  void VulkanRenderer::beginDraw() noexcept
  {
    updateImgIdx();

    CurrentCmdBuff = CmdBuffs[CurrentBuffIdx];

    auto CmdBuffBeginInfo             = VkCommandBufferBeginInfo();
    CmdBuffBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CmdBuffBeginInfo.flags            = 0;
    CmdBuffBeginInfo.pInheritanceInfo = nullptr;

    vkBeginCommandBuffer( CurrentCmdBuff, &CmdBuffBeginInfo );

    auto ClrColorVal  = VkClearValue();
    ClrColorVal.color = { { 0.0F, 0.0F, 0.0F, 1.0F } };

    auto ClrDepthVal         = VkClearValue();
    ClrDepthVal.depthStencil = { 1.0F, 0 };

    auto const ClrVals = std::array{ ClrColorVal, ClrDepthVal };

    auto RenderPassBeginInfo                = VkRenderPassBeginInfo();
    RenderPassBeginInfo.sType               = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.renderPass          = RenderPass;
    RenderPassBeginInfo.framebuffer         = Framebuffers[CurrentImgIdx];
    RenderPassBeginInfo.renderArea.offset.x = 0;
    RenderPassBeginInfo.renderArea.offset.y = 0;
    RenderPassBeginInfo.renderArea.extent   = SwapchainExtent;
    RenderPassBeginInfo.clearValueCount     = static_cast<uint32_t>( std::size( ClrVals ) );
    RenderPassBeginInfo.pClearValues        = std::data( ClrVals );

    vkCmdBeginRenderPass( CurrentCmdBuff, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
  }

  void drawModel( ModelID ID ) noexcept;

  void VulkanRenderer::endDraw() noexcept
  {
    vkCmdEndRenderPass( CurrentCmdBuff );
    vkEndCommandBuffer( CurrentCmdBuff );

    auto const Device = VulkanContext::the().getDevice();

    // Wait for the Img in flight to end if it is

    auto const ImgInFlightFence = ImgInFlightFences[CurrentImgIdx];

    if ( ImgInFlightFence.has_value() )
    {
      vkWaitForFences( Device, 1, &ImgInFlightFence.value(), VK_TRUE, std::numeric_limits<int64_t>::max() );
    }

    ImgInFlightFences[CurrentImgIdx] = FrameInFlightFences[CurrentFrameIdx];

    // get current semaphores
    auto const ImgAvailableSemaphore   = ImgAvailableSemaphores[CurrentFrameIdx];
    auto const RenderFinishedSemaphore = RenderFinishedSemaphores[CurrentFrameIdx];

    auto const WaitSemaphore  = std::array{ ImgAvailableSemaphore };
    auto const SigSemaphore   = std::array{ RenderFinishedSemaphore };
    auto const WaitStages     = std::array<VkPipelineStageFlags, 1>{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    auto const SubmitCmdBuffs = std::array{ CurrentCmdBuff };

    auto SubmitInfo                 = VkSubmitInfo();
    SubmitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.waitSemaphoreCount   = static_cast<uint32_t>( std::size( WaitSemaphore ) );
    SubmitInfo.pWaitSemaphores      = std::data( WaitSemaphore );
    SubmitInfo.pWaitDstStageMask    = std::data( WaitStages );
    SubmitInfo.commandBufferCount   = static_cast<uint32_t>( std::size( SubmitCmdBuffs ) );
    SubmitInfo.pCommandBuffers      = std::data( SubmitCmdBuffs );
    SubmitInfo.signalSemaphoreCount = static_cast<uint32_t>( std::size( SigSemaphore ) );
    SubmitInfo.pSignalSemaphores    = std::data( SigSemaphore );

    auto const FrameInFlightFence = FrameInFlightFences[CurrentFrameIdx];
    auto const GfxQueue           = VulkanContext::the().getGraphicsQueue();

    vkResetFences( Device, 1, &FrameInFlightFence );
    vkQueueSubmit( GfxQueue, 1, &SubmitInfo, FrameInFlightFence );

    auto const PresentSignalSemaphore = std::array{ RenderFinishedSemaphore };
    auto const Swapchains             = std::array{ Swapchain };
    auto const ImgIdxs                = std::array{ CurrentImgIdx };

    auto PresentInfo               = VkPresentInfoKHR();
    PresentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    PresentInfo.waitSemaphoreCount = static_cast<uint32_t>( std::size( PresentSignalSemaphore ) );
    PresentInfo.pWaitSemaphores    = std::data( PresentSignalSemaphore );
    PresentInfo.swapchainCount     = static_cast<uint32_t>( std::size( Swapchains ) );
    PresentInfo.pSwapchains        = std::data( Swapchains );
    PresentInfo.pImageIndices      = std::data( ImgIdxs );
    PresentInfo.pResults           = nullptr;

    auto const PresentQueue = VulkanContext::the().getPresentQueue();

    auto Result = vkQueuePresentKHR( PresentQueue, &PresentInfo );
    vkQueueWaitIdle( PresentQueue );

    auto const FramebufferResized = VulkanContext::the().getIsFramebufferResized();
    auto const ChangeSwapchain    = ( Result == VK_ERROR_OUT_OF_DATE_KHR ) || ( Result == VK_SUBOPTIMAL_KHR );

    if ( ChangeSwapchain || FramebufferResized )
    {
      VulkanContext::the().setIsFramebufferResized( false );
      recreateAfterFramebufferChange();
      return;
    }

    MVK_VERIFY( VK_SUCCESS == Result );
    CurrentFrameIdx = ( CurrentFrameIdx + 1 ) % MaxFramesInFlight;
    CurrentBuffIdx  = ( CurrentBuffIdx + 1 ) % DynamicBuffCount;
  }

  void VulkanRenderer::updateImgIdx() noexcept
  {
    auto           Idx                   = uint32_t( 0 );
    auto const     ImgAvailableSemaphore = ImgAvailableSemaphores[CurrentFrameIdx];
    constexpr auto Timeout               = std::numeric_limits<uint64_t>::max();

    auto const Device = VulkanContext::the().getDevice();

    while ( vkAcquireNextImageKHR( Device, Swapchain, Timeout, ImgAvailableSemaphore, nullptr, &Idx ) == VK_ERROR_OUT_OF_DATE_KHR ) {}

    CurrentImgIdx = Idx;
  }

  void VulkanRenderer::drawModel( ModelID ID ) noexcept
  {
    auto & Model   = Models[ID];
    auto   DescSet = ModelDescSets[ID];

    auto VtxBuff = Model->Vbo.getBuff();
    auto IdxBuff = Model->Ibo.getBuff();

    auto Pvm = createTestPvm();
    Model->Ubo.map( { reinterpret_cast<std::byte const *>( &Pvm ), sizeof( PVM ) } );

    auto VtxOff = VkDeviceSize( 0 );

    vkCmdBindPipeline( CurrentCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPipeline );
    vkCmdBindVertexBuffers( CurrentCmdBuff, 0, 1, &VtxBuff, &VtxOff );
    vkCmdBindIndexBuffer( CurrentCmdBuff, IdxBuff, 0, VK_INDEX_TYPE_UINT32 );
    vkCmdBindDescriptorSets( CurrentCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPipelineLayout, 0, 1, &DescSet, 0, nullptr );
    vkCmdDrawIndexed( CurrentCmdBuff, Model->Ibo.getCnt(), 1, 0, 0, 0 );
  }

}  // namespace Mvk::Engine
