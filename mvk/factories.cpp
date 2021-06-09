#include "factories.hpp"

#include "utility/misc.hpp"

#include <array>
#include <optional>
#include <utility>

namespace mvk::factories
{

namespace detail
{

[[nodiscard]] static constexpr uint32_t
choose_image_count(VkSurfaceCapabilitiesKHR const & capabilities) noexcept;

[[nodiscard]] static VkPresentModeKHR
choose_present_mode(
  VkPhysicalDevice physical_device,
  VkSurfaceKHR     surface) noexcept;

[[nodiscard]] static constexpr VkExtent2D
choose_extent(
  VkSurfaceCapabilitiesKHR const & capabilities,
  VkExtent2D const &               extent) noexcept;

} // namespace detail

[[nodiscard]] vk_types::buffer
create_buffer(
  vk_types::device const & device,
  VkDeviceSize const       size,
  VkBufferUsageFlags const usage,
  VkSharingMode const      sharing_mode)
{
  auto info        = VkBufferCreateInfo();
  info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  info.size        = size;
  info.usage       = usage;
  info.sharingMode = sharing_mode;

  return vk_types::buffer(device.get(), info);
}

[[nodiscard]] vk_types::command_buffers
create_command_buffers(
  vk_types::command_pool const & command_pool,
  VkDeviceSize const             size)
{
  auto info               = VkCommandBufferAllocateInfo();
  info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  info.commandPool        = command_pool.get();
  info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  info.commandBufferCount = static_cast<uint32_t>(size);

  return vk_types::command_buffers(command_pool.parent(), info);
}

[[nodiscard]] vk_types::command_pool
create_command_pool(vk_types::device const & device)
{
  auto const [graphics_queue, present_queue] = device.get_queues();

  auto info             = VkCommandPoolCreateInfo();
  info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  info.queueFamilyIndex = graphics_queue.index();
  info.flags            = 0;
  return vk_types::command_pool(device.get(), info);
}

[[nodiscard]] vk_types::descriptor_pool
create_descriptor_pool(vk_types::device const & device, size_t const max_sets)
{
  auto const max_sets_value = static_cast<uint32_t>(max_sets);

  auto const pool_sizes = std::array{
    [max_sets_value]
    {
      auto uniform_pool_size            = VkDescriptorPoolSize();
      uniform_pool_size.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      uniform_pool_size.descriptorCount = max_sets_value;
      return uniform_pool_size;
    }(),
    [max_sets_value]
    {
      auto sampler_pool_size = VkDescriptorPoolSize();
      sampler_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      sampler_pool_size.descriptorCount = max_sets_value;
      return sampler_pool_size;
    }()};

  auto const [data, size] = utility::bind_data_and_size(pool_sizes);
  auto info               = VkDescriptorPoolCreateInfo();
  info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  info.poolSizeCount      = static_cast<uint32_t>(size);
  info.pPoolSizes         = data;
  info.maxSets            = max_sets_value;
  info.flags              = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

  return vk_types::descriptor_pool(device.get(), info);
}

[[nodiscard]] vk_types::descriptor_set_layout
create_descriptor_set_layout(vk_types::device const & device)
{
  auto const bindings = std::array{
    []
    {
      auto ubo               = VkDescriptorSetLayoutBinding();
      ubo.binding            = 0;
      ubo.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      ubo.descriptorCount    = 1;
      ubo.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
      ubo.pImmutableSamplers = nullptr;
      return ubo;
    }(),
    []
    {
      auto sampler               = VkDescriptorSetLayoutBinding();
      sampler.binding            = 1;
      sampler.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      sampler.descriptorCount    = 1;
      sampler.pImmutableSamplers = nullptr;
      sampler.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
      return sampler;
    }()};

  auto const [data, size] = utility::bind_data_and_size(bindings);

  auto info         = VkDescriptorSetLayoutCreateInfo();
  info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.bindingCount = static_cast<uint32_t>(size);
  info.pBindings    = data;

  return vk_types::descriptor_set_layout(device.get(), info);
}

[[nodiscard]] vk_types::descriptor_sets
create_descriptor_sets(
  vk_types::descriptor_pool const &       descriptor_pool,
  vk_types::descriptor_set_layout const & descriptor_set_layout,
  vk_types::swapchain const &             swapchain)
{
  auto const images_size = std::size(swapchain.images());

  auto layouts = std::vector(images_size, descriptor_set_layout.get());
  auto const [layout_data, layout_size] = utility::bind_data_and_size(layouts);

  auto info               = VkDescriptorSetAllocateInfo();
  info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  info.descriptorPool     = descriptor_pool.get();
  info.descriptorSetCount = static_cast<uint32_t>(layout_size);
  info.pSetLayouts        = layout_data;

  auto const parent =
    find_parent(descriptor_pool, descriptor_set_layout, swapchain);

  return vk_types::descriptor_sets(parent, info);
}

[[nodiscard]] vk_types::fence
create_fence(vk_types::device const & device)
{
  auto info  = VkFenceCreateInfo();
  info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  return vk_types::fence(device.get(), info);
}

[[nodiscard]] vk_types::framebuffer
create_framebuffer(
  vk_types::swapchain const &   swapchain,
  vk_types::render_pass const & render_pass,
  vk_types::image_view const &  current_image_view,
  vk_types::image_view const &  depth_image_view)
{
  auto const extent = swapchain.extent();
  auto const attachments =
    std::array{current_image_view.get(), depth_image_view.get()};

  auto const [attachment_data, attachment_size] =
    utility::bind_data_and_size(attachments);

  auto info            = VkFramebufferCreateInfo();
  info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  info.renderPass      = render_pass.get();
  info.attachmentCount = static_cast<uint32_t>(attachment_size);
  info.pAttachments    = attachment_data;
  info.width           = extent.width;
  info.height          = extent.height;
  info.layers          = 1;

  auto const parent =
    find_parent(swapchain, render_pass, current_image_view, depth_image_view);
  return vk_types::framebuffer(parent, info);
}

[[nodiscard]] vk_types::image_view
create_image_view(
  vk_types::device const &  device,
  vk_types::surface const & surface,
  VkImage const             image)
{
  auto info         = VkImageViewCreateInfo();
  info.sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  info.image        = image;
  info.viewType     = VK_IMAGE_VIEW_TYPE_2D;
  info.format       = surface.find_format(device.physical_device()).format;
  info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  info.subresourceRange.baseMipLevel   = 0;
  info.subresourceRange.levelCount     = 1;
  info.subresourceRange.baseArrayLayer = 0;
  info.subresourceRange.layerCount     = 1;

  return vk_types::image_view(device.get(), info);
}

[[nodiscard]] vk_types::image_view
create_image_view(
  vk_types::image const &  image,
  VkFormat const           format,
  VkImageAspectFlags const aspect)
{
  auto info                        = VkImageViewCreateInfo();
  info.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  info.image                       = image.get();
  info.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
  info.format                      = format;
  info.components.r                = VK_COMPONENT_SWIZZLE_IDENTITY;
  info.components.g                = VK_COMPONENT_SWIZZLE_IDENTITY;
  info.components.b                = VK_COMPONENT_SWIZZLE_IDENTITY;
  info.components.a                = VK_COMPONENT_SWIZZLE_IDENTITY;
  info.subresourceRange.aspectMask = aspect;
  info.subresourceRange.baseMipLevel   = 0;
  info.subresourceRange.levelCount     = image.mipmap_levels();
  info.subresourceRange.baseArrayLayer = 0;
  info.subresourceRange.layerCount     = 1;

  return vk_types::image_view(image.parent(), info);
}

[[nodiscard]] vk_types::image
create_image(
  vk_types::device const & device,
  uint32_t const           width,
  uint32_t const           height,
  VkFormat const           format,
  VkImageTiling const      tiling,
  VkImageUsageFlags const  usage,
  VkSharingMode const      sharing_mode)
{
  auto info          = VkImageCreateInfo();
  info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  info.imageType     = VK_IMAGE_TYPE_2D;
  info.extent.width  = width;
  info.extent.height = height;
  info.extent.depth  = 1;
  info.mipLevels     = 1;
  info.arrayLayers   = 1;
  info.format        = format;
  info.tiling        = tiling;
  info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  info.usage         = usage;
  info.sharingMode   = sharing_mode;
  info.samples       = VK_SAMPLE_COUNT_1_BIT;
  info.flags         = 0;

  return vk_types::image(device.get(), info);
}

[[nodiscard]] vk_types::image
create_image(
  vk_types::device const &         device,
  vk_types::image::texture const & texture)
{
  auto usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

  auto info          = VkImageCreateInfo();
  info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  info.imageType     = VK_IMAGE_TYPE_2D;
  info.extent.width  = texture.width();
  info.extent.height = texture.height();
  info.extent.depth  = 1;
  info.mipLevels     = texture.mipmap_levels();
  info.arrayLayers   = 1;
  info.format        = VK_FORMAT_R8G8B8A8_SRGB;
  info.tiling        = VK_IMAGE_TILING_OPTIMAL;
  info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  info.usage         = static_cast<VkFlags>(usage);
  info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
  info.samples       = VK_SAMPLE_COUNT_1_BIT;
  info.flags         = 0;

  return vk_types::image(device.get(), info);
}
[[nodiscard]] vk_types::instance
create_instance(
  std::string const &                name,
  utility::slice<char const *> const required_extensions)
{
  auto app_info               = VkApplicationInfo();
  app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName   = name.c_str();
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName        = "No Engine";
  app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);

  using vk_types::validation::validation_layers_data;
  auto const validation_layers = validation_layers_data();
  auto const [layer_names, layer_count] =
    utility::bind_data_and_size(validation_layers);

  auto const [required_extensions_data, required_extensions_size] =
    utility::bind_data_and_size(required_extensions);

  auto info                  = VkInstanceCreateInfo();
  info.sType                 = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  info.pNext                 = vk_types::validation::debug_create_info_ref();
  info.pApplicationInfo      = &app_info;
  info.enabledLayerCount     = static_cast<uint32_t>(layer_count);
  info.ppEnabledLayerNames   = layer_names;
  info.enabledExtensionCount = static_cast<uint32_t>(required_extensions_size);
  info.ppEnabledExtensionNames = required_extensions_data;

  return vk_types::instance(info);
}

[[nodiscard]] vk_types::pipeline_layout
create_pipeline_layout(
  vk_types::descriptor_set_layout const & descriptor_set_layout)
{
  auto const layout = descriptor_set_layout.get();

  auto info                   = VkPipelineLayoutCreateInfo();
  info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  info.setLayoutCount         = 1;
  info.pSetLayouts            = &layout;
  info.pushConstantRangeCount = 0;
  info.pPushConstantRanges    = nullptr;

  return vk_types::pipeline_layout(descriptor_set_layout.parent(), info);
}

[[nodiscard]] vk_types::pipeline
create_pipeline(
  vk_types::swapchain const &                           swapchain,
  vk_types::render_pass const &                         render_pass,
  vk_types::pipeline_layout const &                     layout,
  utility::slice<VkPipelineShaderStageCreateInfo> const shader_stages)
{
  auto const vertex_input_binding_description = []
  {
    auto description      = VkVertexInputBindingDescription();
    description.binding   = 0;
    description.stride    = sizeof(vertex);
    description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return description;
  }();

  auto const vertex_attributes = std::array{
    []
    {
      auto tmp     = VkVertexInputAttributeDescription();
      tmp.binding  = 0;
      tmp.location = 0;
      tmp.format   = VK_FORMAT_R32G32B32_SFLOAT;
      tmp.offset   = offsetof(vertex, pos);
      return tmp;
    }(),
    []
    {
      auto tmp     = VkVertexInputAttributeDescription();
      tmp.binding  = 0;
      tmp.location = 1;
      tmp.format   = VK_FORMAT_R32G32B32_SFLOAT;
      tmp.offset   = offsetof(vertex, color);
      return tmp;
    }(),
    []
    {
      auto tmp     = VkVertexInputAttributeDescription();
      tmp.binding  = 0;
      tmp.location = 2;
      tmp.format   = VK_FORMAT_R32G32_SFLOAT;
      tmp.offset   = offsetof(vertex, texture_coord);
      return tmp;
    }()};

  auto const vertex_input_create_info =
    [&vertex_input_binding_description, &vertex_attributes]
  {
    auto const count = static_cast<uint32_t>(std::size(vertex_attributes));

    auto info  = VkPipelineVertexInputStateCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info.vertexBindingDescriptionCount   = 1;
    info.pVertexBindingDescriptions      = &vertex_input_binding_description;
    info.vertexAttributeDescriptionCount = count;
    info.pVertexAttributeDescriptions    = std::data(vertex_attributes);
    return info;
  }();

  auto const input_assembly_create_info = []
  {
    auto info  = VkPipelineInputAssemblyStateCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    info.primitiveRestartEnable = VK_FALSE;
    return info;
  }();

  auto const extent = swapchain.extent();

  auto const viewport = [&extent]
  {
    auto tmp     = VkViewport();
    tmp.x        = 0.0F;
    tmp.y        = 0.0F;
    tmp.width    = static_cast<float>(extent.width);
    tmp.height   = static_cast<float>(extent.height);
    tmp.minDepth = 0.0F;
    tmp.maxDepth = 1.0F;
    return tmp;
  }();

  auto const scissor = [&extent]
  {
    auto tmp     = VkRect2D();
    tmp.offset.x = 0;
    tmp.offset.y = 0;
    tmp.extent   = extent;
    return tmp;
  }();

  auto const viewport_state_create_info = [&viewport, &scissor]
  {
    auto info          = VkPipelineViewportStateCreateInfo();
    info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    info.viewportCount = 1;
    info.pViewports    = &viewport;
    info.scissorCount  = 1;
    info.pScissors     = &scissor;
    return info;
  }();

  auto const rasterizer_create_info = []
  {
    auto info  = VkPipelineRasterizationStateCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info.depthClampEnable        = VK_FALSE;
    info.rasterizerDiscardEnable = VK_FALSE;
    info.polygonMode             = VK_POLYGON_MODE_FILL;
    info.lineWidth               = 1.0F;
    info.cullMode                = VK_CULL_MODE_BACK_BIT;
    info.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    info.depthBiasEnable         = VK_FALSE;
    info.depthBiasConstantFactor = 0.0F;
    info.depthBiasClamp          = 0.0F;
    info.depthBiasSlopeFactor    = 0.0F;
    return info;
  }();

  auto const multisampling_create_info = []
  {
    auto info  = VkPipelineMultisampleStateCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info.sampleShadingEnable   = VK_FALSE;
    info.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    info.minSampleShading      = 1.0F;
    info.pSampleMask           = nullptr;
    info.alphaToCoverageEnable = VK_FALSE;
    info.alphaToOneEnable      = VK_FALSE;
    return info;
  }();

  auto const color_blend_attachment = []
  {
    auto attachment = VkPipelineColorBlendAttachmentState();
    attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    attachment.blendEnable         = VK_FALSE;
    attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    attachment.colorBlendOp        = VK_BLEND_OP_ADD;
    attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    attachment.alphaBlendOp        = VK_BLEND_OP_ADD;
    return attachment;
  }();

  auto const color_blend_create_info = [&color_blend_attachment]
  {
    auto info  = VkPipelineColorBlendStateCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    info.logicOpEnable     = VK_FALSE;
    info.logicOp           = VK_LOGIC_OP_COPY;
    info.attachmentCount   = 1;
    info.pAttachments      = &color_blend_attachment;
    info.blendConstants[0] = 0.0F;
    info.blendConstants[1] = 0.0F;
    info.blendConstants[2] = 0.0F;
    info.blendConstants[3] = 0.0F;
    return info;
  }();

  auto const depth_stencil_state_create_info = []
  {
    auto info  = VkPipelineDepthStencilStateCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    info.depthTestEnable       = VK_TRUE;
    info.depthWriteEnable      = VK_TRUE;
    info.depthCompareOp        = VK_COMPARE_OP_LESS;
    info.depthBoundsTestEnable = VK_FALSE;
    info.minDepthBounds        = 0.0F;
    info.maxDepthBounds        = 1.0F;
    info.stencilTestEnable     = VK_FALSE;
    return info;
  }();

  auto info                = VkGraphicsPipelineCreateInfo();
  info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  info.stageCount          = static_cast<uint32_t>(std::size(shader_stages));
  info.pStages             = std::data(shader_stages);
  info.pVertexInputState   = &vertex_input_create_info;
  info.pInputAssemblyState = &input_assembly_create_info;
  info.pViewportState      = &viewport_state_create_info;
  info.pRasterizationState = &rasterizer_create_info;
  info.pMultisampleState   = &multisampling_create_info;
  info.pDepthStencilState  = &depth_stencil_state_create_info;
  info.pColorBlendState    = &color_blend_create_info;
  info.pDynamicState       = nullptr;
  info.layout              = layout.get();
  info.renderPass          = render_pass.get();
  info.subpass             = 0;
  info.basePipelineHandle  = nullptr;
  info.basePipelineIndex   = -1;

  auto const parent = find_parent(swapchain, render_pass, layout);
  return vk_types::pipeline(parent, info);
}

[[nodiscard]] vk_types::render_pass
create_render_pass(
  vk_types::device const &  device,
  vk_types::surface const & surface)
{
  auto const format = surface.find_format(device.physical_device()).format;

  auto const color_attachment = [&format]
  {
    auto attachment           = VkAttachmentDescription();
    attachment.format         = format;
    attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    return attachment;
  }();

  auto const depth_attachment = []
  {
    auto attachment           = VkAttachmentDescription();
    attachment.format         = VK_FORMAT_D32_SFLOAT;
    attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    return attachment;
  }();

  auto const color_attachment_reference = []
  {
    auto reference       = VkAttachmentReference();
    reference.attachment = 0;
    reference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    return reference;
  }();

  auto const depth_attachment_reference = []
  {
    auto reference       = VkAttachmentReference();
    reference.attachment = 1;
    reference.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    return reference;
  }();

  auto const subpass_description =
    [&color_attachment_reference, &depth_attachment_reference]
  {
    auto description                    = VkSubpassDescription();
    description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    description.colorAttachmentCount    = 1;
    description.pColorAttachments       = &color_attachment_reference;
    description.pDepthStencilAttachment = &depth_attachment_reference;
    return description;
  }();

  auto subpass_dependency = []
  {
    auto dependency          = VkSubpassDependency();
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    return dependency;
  }();

  auto const attachments = std::array{color_attachment, depth_attachment};

  auto info            = VkRenderPassCreateInfo();
  info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = static_cast<uint32_t>(std::size(attachments));
  info.pAttachments    = std::data(attachments);
  info.subpassCount    = 1;
  info.pSubpasses      = &subpass_description;
  info.dependencyCount = 1;
  info.pDependencies   = &subpass_dependency;

  return vk_types::render_pass(device.get(), info);
}

[[nodiscard]] vk_types::sampler
create_sampler(vk_types::device const & device, uint32_t const mipmap_levels)
{
  auto info                    = VkSamplerCreateInfo();
  info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  info.magFilter               = VK_FILTER_LINEAR;
  info.minFilter               = VK_FILTER_LINEAR;
  info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  info.anisotropyEnable        = VK_TRUE;
  info.anisotropyEnable        = VK_TRUE;
  info.maxAnisotropy           = 16;
  info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  info.unnormalizedCoordinates = VK_FALSE;
  info.compareEnable           = VK_FALSE;
  info.compareOp               = VK_COMPARE_OP_ALWAYS;
  info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  info.mipLodBias              = 0.0F;
  info.minLod                  = 0.0F;
  info.maxLod                  = static_cast<float>(mipmap_levels);

  return vk_types::sampler(device.get(), info);
}

[[nodiscard]] vk_types::semaphore
create_semaphore(vk_types::device const & device)
{
  auto info  = VkSemaphoreCreateInfo();
  info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  return vk_types::semaphore(device.get(), info);
}

[[nodiscard]] vk_types::shader_module
create_shader_module(
  vk_types::device const & device,
  std::string_view const   code)
{
  auto info     = VkShaderModuleCreateInfo();
  info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.codeSize = static_cast<uint32_t>(std::size(code));
  info.pCode    = reinterpret_cast<uint32_t const *>(std::data(code));
  return vk_types::shader_module(device.get(), info);
}

[[nodiscard]] vk_types::surface
create_surface(vk_types::instance const & instance, GLFWwindow * const window)
{
  return vk_types::surface(instance.get(), window);
}

[[nodiscard]] vk_types::swapchain
create_swapchain(
  vk_types::device const &  device,
  vk_types::surface const & surface,
  VkExtent2D const          extent)
{
  auto const & surface_handle         = surface.get();
  auto const & physical_device_handle = device.physical_device();

  // FIXME(samuel): shouldn't need to call so many times format
  auto const surface_format = surface.find_format(physical_device_handle);

  auto const present_mode =
    detail::choose_present_mode(physical_device_handle, surface_handle);

  auto const [graphics_queue, present_queue] = device.get_queues();
  auto const graphics_index                  = graphics_queue.index();
  auto const present_index                   = present_queue.index();
  auto const family_indices = std::array{graphics_index, present_index};

  auto capabilities = VkSurfaceCapabilitiesKHR();
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    physical_device_handle,
    surface_handle,
    &capabilities);

  auto const swapchain_extent = detail::choose_extent(capabilities, extent);
  auto const image_count      = detail::choose_image_count(capabilities);

  auto info             = VkSwapchainCreateInfoKHR();
  info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  info.surface          = surface_handle;
  info.minImageCount    = image_count;
  info.imageFormat      = surface_format.format;
  info.imageColorSpace  = surface_format.colorSpace;
  info.imageExtent      = swapchain_extent;
  info.imageArrayLayers = 1;
  info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  info.preTransform     = capabilities.currentTransform;
  info.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  info.presentMode      = present_mode;
  info.clipped          = VK_TRUE;
  info.oldSwapchain     = nullptr;

  if (graphics_index != present_index)
  {
    info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    info.queueFamilyIndexCount = 2;
    info.pQueueFamilyIndices   = std::data(family_indices);
  }
  else
  {
    info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    info.queueFamilyIndexCount = 0;
    info.pQueueFamilyIndices   = nullptr;
  }

  return vk_types::swapchain(device.get(), info);
}

[[nodiscard]] vk_types::device
create_device(
  vk_types::surface const &          surface,
  utility::slice<char const *> const device_extensions) noexcept
{
  auto const physical_device_handle = detail::choose_physical_device(
    surface.parent(),
    surface.get(),
    device_extensions);

  auto const indices =
    detail::query_family_indices(physical_device_handle, surface.get())
      .value();

  auto const [graphics_index, present_index] = indices;

  auto features = VkPhysicalDeviceFeatures();
  vkGetPhysicalDeviceFeatures(physical_device_handle, &features);

  auto queue_priority = 1.0F;

  auto const graphics_queue_create_info = [&queue_priority, &indices]
  {
    auto const graphics_index = indices.first;

    auto info             = VkDeviceQueueCreateInfo();
    info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    info.queueFamilyIndex = graphics_index;
    info.queueCount       = 1;
    info.pQueuePriorities = &queue_priority;
    return info;
  }();

  auto const present_queue_create_info = [&queue_priority, &indices]
  {
    auto const present_index = indices.second;

    auto info             = VkDeviceQueueCreateInfo();
    info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    info.queueFamilyIndex = present_index;
    info.queueCount       = 1;
    info.pQueuePriorities = &queue_priority;
    return info;
  }();

  auto const queue_create_infos =
    std::array{graphics_queue_create_info, present_queue_create_info};

  auto const queue_create_info_count =
    static_cast<uint32_t>(graphics_index != present_index ? 2 : 1);

  using vk_types::validation::validation_layers_data;
  auto const validation_layers = validation_layers_data();
  auto const [layer_names, layer_count] =
    utility::bind_data_and_size(validation_layers);

  auto const enabled_extension_count =
    static_cast<uint32_t>(std::size(device_extensions));

  auto info                    = VkDeviceCreateInfo();
  info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  info.queueCreateInfoCount    = queue_create_info_count;
  info.pQueueCreateInfos       = std::data(queue_create_infos);
  info.pEnabledFeatures        = &features;
  info.enabledExtensionCount   = enabled_extension_count;
  info.ppEnabledExtensionNames = std::data(device_extensions);
  info.enabledLayerCount       = static_cast<uint32_t>(layer_count);
  info.ppEnabledLayerNames     = layer_names;

  return vk_types::device(physical_device_handle, info);
}

namespace detail
{

[[nodiscard]] static constexpr uint32_t
choose_image_count(VkSurfaceCapabilitiesKHR const & capabilities) noexcept
{
  auto const candidate = capabilities.minImageCount + 1;
  auto const max_limit = capabilities.maxImageCount;

  if (max_limit > 0 && candidate > max_limit)
  {
    return max_limit;
  }

  return candidate;
}

[[nodiscard]] static VkPresentModeKHR
choose_present_mode(
  VkPhysicalDevice const physical_device,
  VkSurfaceKHR const     surface) noexcept
{
  auto count = uint32_t(0);
  vkGetPhysicalDeviceSurfacePresentModesKHR(
    physical_device,
    surface,
    &count,
    nullptr);

  auto modes = std::vector<VkPresentModeKHR>(count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(
    physical_device,
    surface,
    &count,
    std::data(modes));

  auto const it =
    std::find(std::begin(modes), std::end(modes), VK_PRESENT_MODE_MAILBOX_KHR);

  if (it != std::end(modes))
  {
    return *it;
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

[[nodiscard]] static constexpr VkExtent2D
choose_extent(
  VkSurfaceCapabilitiesKHR const & capabilities,
  VkExtent2D const &               extent) noexcept
{
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
  {
    return capabilities.currentExtent;
  }

  auto const min_width  = capabilities.minImageExtent.width;
  auto const min_height = capabilities.minImageExtent.height;
  auto const max_width  = capabilities.maxImageExtent.width;
  auto const max_height = capabilities.maxImageExtent.height;

  auto const extent_width  = std::clamp(extent.width, min_width, max_width);
  auto const extent_height = std::clamp(extent.height, min_height, max_height);

  return {extent_width, extent_height};
}

[[nodiscard]] bool
is_extension_present(
  std::string const &                         current_extension_name,
  utility::slice<VkExtensionProperties> const extensions) noexcept
{
  auto const matches = [&current_extension_name](auto const & extension)
  {
    auto const ext_name = static_cast<char const *>(extension.extensionName);
    return std::strcmp(current_extension_name.c_str(), ext_name) == 0;
  };
  return std::any_of(std::begin(extensions), std::end(extensions), matches);
}

[[nodiscard]] bool
check_extension_support(
  VkPhysicalDevice const             physical_device,
  utility::slice<char const *> const device_extensions) noexcept
{
  auto count = uint32_t(0);
  vkEnumerateDeviceExtensionProperties(
    physical_device,
    nullptr,
    &count,
    nullptr);

  auto extensions = std::vector<VkExtensionProperties>(count);
  vkEnumerateDeviceExtensionProperties(
    physical_device,
    nullptr,
    &count,
    std::data(extensions));

  auto is_present = [&extensions](auto const & extension)
  {
    return is_extension_present(extension, extensions);
  };

  auto const begin_extensions = std::begin(device_extensions);
  auto const end_extensions   = std::end(device_extensions);
  return std::all_of(begin_extensions, end_extensions, is_present);
}

[[nodiscard]] bool
check_format_and_present_mode_availability(
  VkPhysicalDevice const physical_device,
  VkSurfaceKHR const     surface) noexcept
{
  auto format_count = uint32_t(0);
  vkGetPhysicalDeviceSurfaceFormatsKHR(
    physical_device,
    surface,
    &format_count,
    nullptr);

  auto present_mode_count = uint32_t(0);
  vkGetPhysicalDeviceSurfacePresentModesKHR(
    physical_device,
    surface,
    &present_mode_count,
    nullptr);

  return format_count != 0 && present_mode_count != 0;
}

[[nodiscard]] constexpr bool
meets_graphic_requirements(
  VkQueueFamilyProperties const & queue_family) noexcept
{
  return (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U;
}

[[nodiscard]] bool
supports_surface(
  VkPhysicalDevice const physical_device,
  VkSurfaceKHR const     surface,
  uint32_t const         index)
{
  auto supported = VkBool32(false);
  vkGetPhysicalDeviceSurfaceSupportKHR(
    physical_device,
    index,
    surface,
    &supported);
  return supported != 0U;
}

[[nodiscard]] std::optional<std::pair<uint32_t, uint32_t>>
query_family_indices(
  VkPhysicalDevice const physical_device,
  VkSurfaceKHR const     surface)
{
  auto count = uint32_t(0);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, nullptr);

  auto queue_families = std::vector<VkQueueFamilyProperties>(count);
  vkGetPhysicalDeviceQueueFamilyProperties(
    physical_device,
    &count,
    std::data(queue_families));

  auto graphics_family = std::optional<uint32_t>();
  auto present_family  = std::optional<uint32_t>();

  auto const is_complete = [&, i = 0U](auto const & queue_family) mutable
  {
    if (queue_family.queueCount == 0)
    {
      ++i;
      return false;
    }

    if (meets_graphic_requirements(queue_family))
    {
      graphics_family = i;
    }

    if (supports_surface(physical_device, surface, i))
    {
      present_family = i;
    }

    auto const families_have_value =
      graphics_family.has_value() && present_family.has_value();

    i += !families_have_value;

    return families_have_value;
  };

  auto const family = std::find_if(
    std::begin(queue_families),
    std::end(queue_families),
    is_complete);

  if (family != std::end(queue_families))
  {
    return std::make_optional(
      std::make_pair(graphics_family.value(), present_family.value()));
  }

  return std::nullopt;
}

[[nodiscard]] VkPhysicalDevice
choose_physical_device(
  VkInstance const                   instance,
  VkSurfaceKHR const                 surface,
  utility::slice<char const *> const device_extensions) noexcept
{
  auto count = uint32_t(0);
  vkEnumeratePhysicalDevices(instance, &count, nullptr);

  auto physical_devices = std::vector<VkPhysicalDevice>(count);
  vkEnumeratePhysicalDevices(instance, &count, std::data(physical_devices));

  auto const supported =
    [device_extensions, surface](auto const physical_device)
  {
    auto features = VkPhysicalDeviceFeatures();
    vkGetPhysicalDeviceFeatures(physical_device, &features);

    return check_extension_support(physical_device, device_extensions) &&
           check_format_and_present_mode_availability(
             physical_device,
             surface) &&
           query_family_indices(physical_device, surface).has_value() &&
           features.samplerAnisotropy;
  };

  auto const dev_start = std::begin(physical_devices);
  auto const dev_end   = std::end(physical_devices);
  auto const it        = std::find_if(dev_start, dev_end, supported);

  if (it != std::end(physical_devices))
  {
    return *it;
  }

  return nullptr;
}

} // namespace detail

} // namespace mvk::factories
