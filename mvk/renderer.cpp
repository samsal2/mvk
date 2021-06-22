#include "renderer.hpp"

#include "detail/creators.hpp"
#include "detail/helpers.hpp"
#include "detail/misc.hpp"
#include "detail/readers.hpp"
#include "utility/misc.hpp"
#include "utility/slice.hpp"
#include "utility/verify.hpp"

#include <limits>

namespace mvk
{
void
renderer::init()
{
  window_ = types::window({600, 600});

  init_vulkan();
  init_swapchain();
  preload_stuff();
  load_mesh();
  init_main_renderpass();
  init_framebuffers();
  init_descriptors();
  init_pipeline();
  init_sync();
}

void
renderer::init_vulkan()
{
  MVK_VERIFY(validation::check_support());

  instance_ = detail::create_instance(window_, "stan loona");

  auto surface = VkSurfaceKHR();
  glfwCreateWindowSurface(types::get(instance_), types::get(window_), nullptr,
                          &surface);

  surface_ = types::unique_surface(
      surface,
      typename types::unique_surface::deleter_type(types::get(instance_)));

  debug_messenger_ =
      types::unique_debug_messenger::create(types::get(instance_));

  auto const physical_device_result = detail::choose_physical_device(
      types::decay(instance_), types::decay(surface_), device_extensions);

  MVK_VERIFY(physical_device_result.has_value());

  physical_device_ = physical_device_result.value();

  auto const queue_indices_result =
      detail::query_family_indices(physical_device_, types::decay(surface_));

  MVK_VERIFY(queue_indices_result.has_value());

  auto const queue_indices = queue_indices_result.value();
  std::tie(graphics_queue_index_, present_queue_index_) = queue_indices;

  auto const features = detail::query<vkGetPhysicalDeviceFeatures>::with(
      types::get(physical_device_));

  auto queue_priority = 1.0F;

  auto const graphics_queue_create_info = [&queue_priority, &queue_indices]
  {
    auto const graphics_index = queue_indices.first;

    auto info = VkDeviceQueueCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    info.queueFamilyIndex = graphics_index;
    info.queueCount = 1;
    info.pQueuePriorities = &queue_priority;
    return info;
  }();

  auto const present_queue_create_info = [&queue_priority, &queue_indices]
  {
    auto const present_index = queue_indices.second;

    auto info = VkDeviceQueueCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    info.queueFamilyIndex = present_index;
    info.queueCount = 1;
    info.pQueuePriorities = &queue_priority;
    return info;
  }();

  auto const queue_create_info =
      std::array{graphics_queue_create_info, present_queue_create_info};

  auto const queue_create_info_count = static_cast<uint32_t>(
      queue_indices.first != queue_indices.second ? 2 : 1);

  auto const validation_layers = validation::validation_layers_data();
  auto const device_create_info = [validation_layers, &queue_create_info,
                                   queue_create_info_count, &features]
  {
    auto const [validation_data, validation_count] =
        utility::bind_data_and_size(validation_layers);
    auto const [extensions_data, extensions_count] =
        utility::bind_data_and_size(device_extensions);

    auto info = VkDeviceCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.queueCreateInfoCount = queue_create_info_count;
    info.pQueueCreateInfos = std::data(queue_create_info);
    info.pEnabledFeatures = &features;
    info.enabledExtensionCount = static_cast<uint32_t>(extensions_count);
    info.ppEnabledExtensionNames = extensions_data;
    info.enabledLayerCount = static_cast<uint32_t>(validation_count);
    info.ppEnabledLayerNames = validation_data;
    return info;
  }();

  device_ = types::unique_device::create(types::get(physical_device_),
                                         device_create_info);

  vkGetDeviceQueue(types::get(device_), graphics_queue_index_, 0,
                   &types::get(graphics_queue_));

  vkGetDeviceQueue(types::get(device_), present_queue_index_, 0,
                   &types::get(present_queue_));

  command_pool_ = detail::create_command_pool(types::decay(device_),
                                              graphics_queue_index_);
}

void
renderer::init_swapchain()
{
  auto const family_indices =
      std::array{graphics_queue_index_, present_queue_index_};

  auto const swapchain_create_info = [this, &family_indices]
  {
    auto const framebuffer_size = window_.query_framebuffer_size();
    auto const width = static_cast<uint32_t>(framebuffer_size.width_);
    auto const height = static_cast<uint32_t>(framebuffer_size.height_);
    auto const format = detail::choose_surface_format(
        types::get(physical_device_), types::get(surface_),
        detail::default_format_checker);
    auto const capabilities =
        detail::query<vkGetPhysicalDeviceSurfaceCapabilitiesKHR>::with(
            types::get(physical_device_), types::get(surface_));

    auto const present_mode =
        detail::choose_present_mode(physical_device_, types::decay(surface_));

    extent_ = detail::choose_extent(capabilities, {width, height});
    auto const image_count = detail::choose_image_count(capabilities);

    auto info = VkSwapchainCreateInfoKHR();
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = surface_.get();
    info.minImageCount = image_count;
    info.imageFormat = format.format;
    info.imageColorSpace = format.colorSpace;
    info.imageExtent = extent_;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    info.preTransform = capabilities.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = present_mode;
    info.clipped = VK_TRUE;
    info.oldSwapchain = nullptr;

    if (family_indices[0] != family_indices[1])
    {
      info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      info.queueFamilyIndexCount = 2;
      info.pQueueFamilyIndices = std::data(family_indices);
    }
    else
    {
      info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      info.queueFamilyIndexCount = 0;
      info.pQueueFamilyIndices = nullptr;
    }

    return info;
  }();

  swapchain_ = types::unique_swapchain::create(types::get(device_),
                                               swapchain_create_info);
  swapchain_images_ = detail::query<vkGetSwapchainImagesKHR>::with(
      types::get(device_), types::get(swapchain_));

  swapchain_image_views_.reserve(std::size(swapchain_images_));

  auto const add_image_view = [this, &swapchain_create_info](auto const image)
  {
    auto const image_view_create_info = [image, &swapchain_create_info]
    {
      auto view_info = VkImageViewCreateInfo();
      view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      view_info.image = image;
      view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      view_info.format = swapchain_create_info.imageFormat;
      view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      view_info.subresourceRange.baseMipLevel = 0;
      view_info.subresourceRange.levelCount = 1;
      view_info.subresourceRange.baseArrayLayer = 0;
      view_info.subresourceRange.layerCount = 1;
      return view_info;
    }();

    swapchain_image_views_.push_back(types::unique_image_view::create(
        types::get(device_), image_view_create_info));
  };

  std::for_each(std::begin(swapchain_images_), std::end(swapchain_images_),
                add_image_view);

  auto const depth_image_create_info = [&swapchain_create_info]
  {
    auto info = VkImageCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.extent.width = swapchain_create_info.imageExtent.width;
    info.extent.height = swapchain_create_info.imageExtent.height;
    info.extent.depth = 1;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.format = VK_FORMAT_D32_SFLOAT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.flags = 0;
    return info;
  }();

  depth_image_ = types::unique_image::create(types::get(device_),
                                             depth_image_create_info);

  depth_image_memory_ = detail::create_device_memory(
      types::decay(device_), physical_device_, types::decay(depth_image_),
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  auto const depth_image_view_create_info = [this]
  {
    auto info = VkImageViewCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = depth_image_.get();
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = VK_FORMAT_D32_SFLOAT;
    info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    return info;
  }();

  depth_image_view_ = types::unique_image_view::create(
      types::get(device_), depth_image_view_create_info);

  detail::transition_layout(
      types::decay(device_), graphics_queue_, types::decay(command_pool_),
      types::decay(depth_image_), VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      depth_image_create_info.mipLevels);
}

void
renderer::preload_stuff()
{
  auto const vertex_code = detail::read_file("../../shaders/vert.spv");
  auto vertex_shader =
      detail::create_shader_module(types::decay(device_), vertex_code);

  auto const fragment_code = detail::read_file("../../shaders/frag.spv");
  auto fragment_shader =
      detail::create_shader_module(types::decay(device_), fragment_code);

  builder_.add_stage(std::move(vertex_shader), VK_SHADER_STAGE_VERTEX_BIT)
      .add_stage(std::move(fragment_shader), VK_SHADER_STAGE_FRAGMENT_BIT);

  std::tie(texture_, width_, height_) =
      detail::load_texture("../../assets/viking_room.png");

  auto const image_create_info = [this]
  {
    auto usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    auto info = VkImageCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.extent.width = width_;
    info.extent.height = height_;
    info.extent.depth = 1;
    info.mipLevels = detail::calculate_mimap_levels(width_, height_);
    info.arrayLayers = 1;
    info.format = VK_FORMAT_R8G8B8A8_SRGB;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.usage = static_cast<VkFlags>(usage);
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.flags = 0;
    return info;
  }();

  image_ =
      types::unique_image::create(types::get(device_), image_create_info);

  image_memory_ = detail::create_device_memory(
      types::decay(device_), physical_device_, types::decay(image_),
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  detail::transition_layout(
      types::decay(device_), graphics_queue_, types::decay(command_pool_),
      types::decay(image_), VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image_create_info.mipLevels);

  detail::stage(types::decay(device_), physical_device_, graphics_queue_,
                types::decay(command_pool_), types::decay(image_),
                utility::as_bytes(texture_), width_, height_);

  detail::generate_mipmaps(types::decay(device_), graphics_queue_,
                           types::decay(command_pool_), types::decay(image_),
                           width_, height_, image_create_info.mipLevels);

  auto const image_view_create_info = [this, &image_create_info]
  {
    auto info = VkImageViewCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image_.get();
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = VK_FORMAT_R8G8B8A8_SRGB;
    info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = image_create_info.mipLevels;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    return info;
  }();

  image_view_ = types::unique_image_view::create(types::get(device_),
                                                 image_view_create_info);

  auto const sampler_create_info = [&image_create_info]
  {
    auto info = VkSamplerCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.anisotropyEnable = VK_TRUE;
    info.anisotropyEnable = VK_TRUE;
    info.maxAnisotropy = 16;
    info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;
    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_ALWAYS;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.mipLodBias = 0.0F;
    info.minLod = 0.0F;
    info.maxLod = static_cast<float>(image_create_info.mipLevels);
    return info;
  }();

  sampler_ =
      types::unique_sampler::create(device_.get(), sampler_create_info);

  vertex_buffer_manager_ = buffer_manager(
      types::decay(device_), physical_device_, types::decay(command_pool_),
      graphics_queue_, buffer_manager::type::vertex);
  index_buffer_manager_ = buffer_manager(
      types::decay(device_), physical_device_, types::decay(command_pool_),
      graphics_queue_, buffer_manager::type::index);
}

void
renderer::init_main_renderpass()
{
  auto const format = detail::choose_surface_format(
                          types::get(physical_device_), types::get(surface_),
                          detail::default_format_checker)
                          .format;

  auto const color_attachment = [&format]
  {
    auto attachment = VkAttachmentDescription();
    attachment.format = format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    return attachment;
  }();

  auto const depth_attachment = []
  {
    auto attachment = VkAttachmentDescription();
    attachment.format = VK_FORMAT_D32_SFLOAT;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    return attachment;
  }();

  auto const color_attachment_reference = []
  {
    auto reference = VkAttachmentReference();
    reference.attachment = 0;
    reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    return reference;
  }();

  auto const depth_attachment_reference = []
  {
    auto reference = VkAttachmentReference();
    reference.attachment = 1;
    reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    return reference;
  }();

  auto const subpass_description =
      [&color_attachment_reference, &depth_attachment_reference]
  {
    auto description = VkSubpassDescription();
    description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    description.colorAttachmentCount = 1;
    description.pColorAttachments = &color_attachment_reference;
    description.pDepthStencilAttachment = &depth_attachment_reference;
    return description;
  }();

  auto subpass_dependency = []
  {
    auto dependency = VkSubpassDependency();
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    return dependency;
  }();

  auto const attachments = std::array{color_attachment, depth_attachment};

  auto const render_pass_create_info =
      [&attachments, &subpass_description, &subpass_dependency]
  {
    auto const [attachments_data, attachments_size] =
        utility::bind_data_and_size(attachments);

    auto info = VkRenderPassCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = static_cast<uint32_t>(attachments_size);
    info.pAttachments = attachments_data;
    info.subpassCount = 1;
    info.pSubpasses = &subpass_description;
    info.dependencyCount = 1;
    info.pDependencies = &subpass_dependency;
    return info;
  }();

  render_pass_ = types::unique_render_pass::create(device_.get(),
                                                   render_pass_create_info);
}

void
renderer::init_framebuffers()
{
  framebuffers_.reserve(std::size(swapchain_image_views_));

  auto const add_framebuffer = [this](auto const & image_view)
  {
    auto const attachments =
        std::array{image_view.get(), depth_image_view_.get()};

    auto const framebuffer_create_info = [this, &attachments]
    {
      auto const [attachments_data, attachments_size] =
          utility::bind_data_and_size(attachments);

      auto info = VkFramebufferCreateInfo();
      info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      info.renderPass = render_pass_.get();
      info.attachmentCount = static_cast<uint32_t>(attachments_size);
      info.pAttachments = attachments_data;
      info.width = extent_.width;
      info.height = extent_.height;
      info.layers = 1;
      return info;
    }();

    framebuffers_.push_back(types::unique_framebuffer::create(
        device_.get(), framebuffer_create_info));
  };
  std::for_each(std::begin(swapchain_image_views_),
                std::end(swapchain_image_views_), add_framebuffer);
}

void
renderer::init_commands()
{
  auto const count = static_cast<uint32_t>(std::size(framebuffers_));

  auto info = VkCommandBufferAllocateInfo();
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  info.commandPool = types::get(command_pool_);
  info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  info.commandBufferCount = count;

  command_buffers_ =
      types::unique_command_buffer::allocate(types::get(device_), info);
}

void
renderer::init_descriptors()
{
  auto const ubo_layout = []
  {
    auto layout = VkDescriptorSetLayoutBinding();
    layout.binding = 0;
    layout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout.descriptorCount = 1;
    layout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layout.pImmutableSamplers = nullptr;
    return layout;
  }();

  auto const sampler_layout = []
  {
    auto layout = VkDescriptorSetLayoutBinding();
    layout.binding = 1;
    layout.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layout.descriptorCount = 1;
    layout.pImmutableSamplers = nullptr;
    layout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    return layout;
  }();

  auto const descriptor_set_bindings = std::array{ubo_layout, sampler_layout};

  auto const descriptor_set_layout_create_info = [&descriptor_set_bindings]
  {
    auto const [descriptor_set_binding_data, descriptor_set_binding_size] =
        utility::bind_data_and_size(descriptor_set_bindings);

    auto info = VkDescriptorSetLayoutCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = static_cast<uint32_t>(descriptor_set_binding_size);
    info.pBindings = descriptor_set_binding_data;
    return info;
  }();

  descriptor_set_layout_ = types::unique_descriptor_set_layout::create(
      device_.get(), descriptor_set_layout_create_info);

  auto const images_count =
      static_cast<uint32_t>(std::size(swapchain_images_));

  auto const uniform_pool_size = [images_count]
  {
    auto pool_size = VkDescriptorPoolSize();
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = images_count;
    return pool_size;
  }();

  auto const sampler_pool_size = [images_count]
  {
    auto pool_size = VkDescriptorPoolSize();
    pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size.descriptorCount = images_count;
    return pool_size;
  }();

  auto const descriptor_pool_sizes =
      std::array{uniform_pool_size, sampler_pool_size};

  auto const descriptor_pool_create_info =
      [&descriptor_pool_sizes, images_count]
  {
    auto const [descriptor_pool_sizes_data, descriptor_pool_sizes_count] =
        utility::bind_data_and_size(descriptor_pool_sizes);

    auto info = VkDescriptorPoolCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = static_cast<uint32_t>(descriptor_pool_sizes_count);
    info.pPoolSizes = descriptor_pool_sizes_data;
    info.maxSets = images_count;
    info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    return info;
  }();

  descriptor_pool_ = types::unique_descriptor_pool::create(
      types::get(device_), descriptor_pool_create_info);

  auto const images_size = std::size(swapchain_images_);

  auto descriptor_set_layouts =
      std::vector(images_size, descriptor_set_layout_.get());

  auto const descriptor_sets_allocate_info = [this, &descriptor_set_layouts]
  {
    auto const [descriptor_set_layouts_data, descriptor_set_layouts_size] =
        utility::bind_data_and_size(descriptor_set_layouts);

    auto info = VkDescriptorSetAllocateInfo();
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool = descriptor_pool_.get();
    info.descriptorSetCount =
        static_cast<uint32_t>(descriptor_set_layouts_size);
    info.pSetLayouts = descriptor_set_layouts_data;
    return info;
  }();

  descriptor_sets_ = types::unique_descriptor_set::allocate(
      device_.get(), descriptor_sets_allocate_info);

  uniform_buffers_.reserve(images_size);
  uniform_buffers_memory_.reserve(images_size);

  for (auto i = size_t(0); i < images_size; ++i)
  {
    auto const uniform_buffer_create_info = []
    {
      auto info = VkBufferCreateInfo();
      info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      info.size = sizeof(pvm);
      info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
      info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      return info;
    }();

    uniform_buffers_.push_back(types::unique_buffer::create(
        types::get(device_), uniform_buffer_create_info));

    auto uniform_buffer_memory = detail::create_device_memory(
        types::decay(device_), physical_device_,
        types::decay(uniform_buffers_.back()),
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    mapped_datas_.push_back(
        detail::map_memory(types::decay(device_),
                           types::decay(uniform_buffer_memory), sizeof(pvm)));
    uniform_buffers_memory_.push_back(std::move(uniform_buffer_memory));
  }

  for (auto i = size_t(0); i < images_size; ++i)
  {
    auto const descriptor_buffer_info = [this, i]
    {
      auto info = VkDescriptorBufferInfo();
      info.buffer = uniform_buffers_[i].get();
      info.offset = 0;
      info.range = sizeof(pvm);
      return info;
    }();

    auto const descriptor_image_info = [this]
    {
      auto info = VkDescriptorImageInfo();
      info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      info.imageView = image_view_.get();
      info.sampler = sampler_.get();
      return info;
    }();

    auto const ubo_write = [this, &descriptor_buffer_info, i]
    {
      auto write = VkWriteDescriptorSet();
      write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet = types::get(descriptor_sets_[i]);
      write.dstBinding = 0;
      write.dstArrayElement = 0;
      write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      write.descriptorCount = 1;
      write.pBufferInfo = &descriptor_buffer_info;
      write.pImageInfo = nullptr;
      write.pTexelBufferView = nullptr;
      return write;
    }();

    auto const image_write = [this, &descriptor_image_info, i]
    {
      auto image = VkWriteDescriptorSet();
      image.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      image.dstSet = types::get(descriptor_sets_[i]);
      image.dstBinding = 1;
      image.dstArrayElement = 0;
      image.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      image.descriptorCount = 1;
      image.pImageInfo = &descriptor_image_info;
      return image;
    }();

    auto const descriptor_writes = std::array{ubo_write, image_write};

    vkUpdateDescriptorSets(
        types::get(device_),
        static_cast<uint32_t>(std::size(descriptor_writes)),
        std::data(descriptor_writes), 0, nullptr);
  }
}

void
renderer::init_pipeline()
{
  auto const descriptor_set_layout = descriptor_set_layout_.get();

  auto const pipeline_layout_create_info = [&descriptor_set_layout]
  {
    auto info = VkPipelineLayoutCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = 1;
    info.pSetLayouts = &descriptor_set_layout;
    info.pushConstantRangeCount = 0;
    info.pPushConstantRanges = nullptr;
    return info;
  }();

  pipeline_layout_ = types::unique_pipeline_layout::create(
      device_.get(), pipeline_layout_create_info);

  auto const vertex_input_binding_description = []
  {
    auto description = VkVertexInputBindingDescription();
    description.binding = 0;
    description.stride = sizeof(vertex);
    description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return description;
  }();

  auto const position_attribute = []
  {
    auto attribute = VkVertexInputAttributeDescription();
    attribute.binding = 0;
    attribute.location = 0;
    attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute.offset = offsetof(vertex, pos);
    return attribute;
  }();

  auto const color_attribute = []
  {
    auto attribute = VkVertexInputAttributeDescription();
    attribute.binding = 0;
    attribute.location = 1;
    attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute.offset = offsetof(vertex, color);
    return attribute;
  }();

  auto const texture_coordinate_attribute = []
  {
    auto attribute = VkVertexInputAttributeDescription();
    attribute.binding = 0;
    attribute.location = 2;
    attribute.format = VK_FORMAT_R32G32_SFLOAT;
    attribute.offset = offsetof(vertex, texture_coord);
    return attribute;
  }();

  auto const vertex_attributes = std::array{
      position_attribute, color_attribute, texture_coordinate_attribute};

  auto const vertex_input_create_info =
      [&vertex_input_binding_description, &vertex_attributes]
  {
    auto const [vertex_attributes_data, vertex_attributes_size] =
        utility::bind_data_and_size(vertex_attributes);

    auto info = VkPipelineVertexInputStateCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info.vertexBindingDescriptionCount = 1;
    info.pVertexBindingDescriptions = &vertex_input_binding_description;
    info.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(vertex_attributes_size);
    info.pVertexAttributeDescriptions = vertex_attributes_data;
    return info;
  }();

  auto const input_assembly_create_info = []
  {
    auto info = VkPipelineInputAssemblyStateCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    info.primitiveRestartEnable = VK_FALSE;
    return info;
  }();

  auto const viewport = [this]
  {
    auto tmp = VkViewport();
    tmp.x = 0.0F;
    tmp.y = 0.0F;
    tmp.width = static_cast<float>(extent_.width);
    tmp.height = static_cast<float>(extent_.height);
    tmp.minDepth = 0.0F;
    tmp.maxDepth = 1.0F;
    return tmp;
  }();

  auto const scissor = [this]
  {
    auto tmp = VkRect2D();
    tmp.offset.x = 0;
    tmp.offset.y = 0;
    tmp.extent = extent_;
    return tmp;
  }();

  auto const viewport_state_create_info = [&viewport, &scissor]
  {
    auto info = VkPipelineViewportStateCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    info.viewportCount = 1;
    info.pViewports = &viewport;
    info.scissorCount = 1;
    info.pScissors = &scissor;
    return info;
  }();

  auto const rasterizer_create_info = []
  {
    auto info = VkPipelineRasterizationStateCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info.depthClampEnable = VK_FALSE;
    info.rasterizerDiscardEnable = VK_FALSE;
    info.polygonMode = VK_POLYGON_MODE_FILL;
    info.lineWidth = 1.0F;
    info.cullMode = VK_CULL_MODE_BACK_BIT;
    info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    info.depthBiasEnable = VK_FALSE;
    info.depthBiasConstantFactor = 0.0F;
    info.depthBiasClamp = 0.0F;
    info.depthBiasSlopeFactor = 0.0F;
    return info;
  }();

  auto const multisampling_create_info = []
  {
    auto info = VkPipelineMultisampleStateCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info.sampleShadingEnable = VK_FALSE;
    info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    info.minSampleShading = 1.0F;
    info.pSampleMask = nullptr;
    info.alphaToCoverageEnable = VK_FALSE;
    info.alphaToOneEnable = VK_FALSE;
    return info;
  }();

  auto const color_blend_attachment = []
  {
    auto attachment = VkPipelineColorBlendAttachmentState();
    attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    attachment.blendEnable = VK_FALSE;
    attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    attachment.colorBlendOp = VK_BLEND_OP_ADD;
    attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    return attachment;
  }();

  auto const color_blend_create_info = [&color_blend_attachment]
  {
    auto info = VkPipelineColorBlendStateCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    info.logicOpEnable = VK_FALSE;
    info.logicOp = VK_LOGIC_OP_COPY;
    info.attachmentCount = 1;
    info.pAttachments = &color_blend_attachment;
    info.blendConstants[0] = 0.0F;
    info.blendConstants[1] = 0.0F;
    info.blendConstants[2] = 0.0F;
    info.blendConstants[3] = 0.0F;
    return info;
  }();

  auto const depth_stencil_state_create_info = []
  {
    auto info = VkPipelineDepthStencilStateCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    info.depthTestEnable = VK_TRUE;
    info.depthWriteEnable = VK_TRUE;
    info.depthCompareOp = VK_COMPARE_OP_LESS;
    info.depthBoundsTestEnable = VK_FALSE;
    info.minDepthBounds = 0.0F;
    info.maxDepthBounds = 1.0F;
    info.stencilTestEnable = VK_FALSE;
    return info;
  }();

  auto const pipeline_create_info = [&, this]
  {
    auto const shader_stages = builder_.stages();
    auto const [shader_stages_data, shader_stages_size] =
        utility::bind_data_and_size(shader_stages);

    auto info = VkGraphicsPipelineCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.stageCount = static_cast<uint32_t>(shader_stages_size);
    info.pStages = shader_stages_data;
    info.pVertexInputState = &vertex_input_create_info;
    info.pInputAssemblyState = &input_assembly_create_info;
    info.pViewportState = &viewport_state_create_info;
    info.pRasterizationState = &rasterizer_create_info;
    info.pMultisampleState = &multisampling_create_info;
    info.pDepthStencilState = &depth_stencil_state_create_info;
    info.pColorBlendState = &color_blend_create_info;
    info.pDynamicState = nullptr;
    info.layout = pipeline_layout_.get();
    info.renderPass = render_pass_.get();
    info.subpass = 0;
    info.basePipelineHandle = nullptr;
    info.basePipelineIndex = -1;
    return info;
  }();

  pipeline_ =
      types::unique_pipeline::create(device_.get(), pipeline_create_info);
}
void
renderer::init_sync()
{
  auto const semaphore_create_info = []
  {
    auto info = VkSemaphoreCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    return info;
  }();

  auto const fence_create_info = []
  {
    auto info = VkFenceCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    return info;
  }();

  for (auto i = size_t(0); i < max_frames_in_flight; ++i)
  {
    image_available_semaphores_[i] =
        types::unique_semaphore::create(device_.get(), semaphore_create_info);
    render_finished_semaphores_[i] =
        types::unique_semaphore::create(device_.get(), semaphore_create_info);
    frame_in_flight_fences_[i] =
        types::unique_fence::create(device_.get(), fence_create_info);
  }

  image_in_flight_fences_.resize(std::size(swapchain_images_), nullptr);
}

void
renderer::load_mesh()
{
  std::tie(vertices_, indices_) =
      detail::read_object("../../assets/viking_room.obj");
}

void
renderer::recreate_after_framebuffer_change()
{
  swapchain_images_.clear();
  swapchain_image_views_.clear();

  init_swapchain();
  init_main_renderpass();

  framebuffers_.clear();

  init_framebuffers();
  init_commands();

  uniform_buffers_.clear();
  uniform_buffers_memory_.clear();
  descriptor_sets_.clear();

  init_descriptors();
  init_pipeline();
  init_sync();
  load_mesh();
}

void
renderer::run()
{
  while (!window_.should_close())
  {
    glfwPollEvents();

    begin_draw();

    auto const pvm = create_test_pvm();

    basic_draw(utility::as_bytes(vertices_), utility::as_bytes(indices_),
               utility::as_bytes(pvm));

    end_draw();
  }

  vkDeviceWaitIdle(types::get(device_));
}

void
renderer::begin_draw()
{
  auto const & image_available_semaphore =
      image_available_semaphores_[current_frame_index_];

  auto const current_image_index = detail::next_swapchain_image(
      types::get(device_), types::get(swapchain_),
      types::get(image_available_semaphore), nullptr);

  if (!current_image_index.has_value())
  {
    recreate_after_framebuffer_change();
    begin_draw();
    return;
  }

  current_image_index_ = current_image_index.value();

  auto clear_color_value = VkClearValue();
  clear_color_value.color = {{0.0F, 0.0F, 0.0F, 1.0F}};

  auto clear_depth_value = VkClearValue();
  clear_depth_value.depthStencil = {1.0F, 0};

  auto const clear_values = std::array{clear_color_value, clear_depth_value};

  auto const render_pass_begin_info = [this, &clear_values]
  {
    auto const [clear_values_data, clear_values_size] =
        utility::bind_data_and_size(clear_values);

    auto info = VkRenderPassBeginInfo();
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = render_pass_.get();
    info.framebuffer = types::get(framebuffers_[current_image_index_]);
    info.renderArea.offset.x = 0;
    info.renderArea.offset.y = 0;
    info.renderArea.extent = extent_;
    info.clearValueCount = static_cast<uint32_t>(clear_values_size);
    info.pClearValues = clear_values_data;
    return info;
  }();

  auto const command_buffer_begin_info = []
  {
    auto info = VkCommandBufferBeginInfo();
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = 0;
    info.pInheritanceInfo = nullptr;
    return info;
  }();

  // Recreate command buffer
  init_commands();

  current_command_buffer_ =
      types::decay(command_buffers_[current_image_index_]);

  vkBeginCommandBuffer(types::get(current_command_buffer_),
                       &command_buffer_begin_info);
  vkCmdBeginRenderPass(types::get(current_command_buffer_),
                       &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}
void
renderer::basic_draw(utility::slice<std::byte> const vertices,
                     utility::slice<std::byte> const indices,
                     utility::slice<std::byte> const pvm)
{
  auto const [vertex_buffer, vertex_offset] =
      vertex_buffer_manager_.map(vertices);
  auto const [index_buffer, index_offset] =
      index_buffer_manager_.map(indices);

  auto data = mapped_datas_[current_image_index_];
  auto pvm_bytes = utility::as_bytes(pvm);

  std::copy(std::begin(pvm_bytes), std::end(pvm_bytes), std::begin(data));

  vkCmdBindPipeline(types::get(current_command_buffer_),
                    VK_PIPELINE_BIND_POINT_GRAPHICS, types::get(pipeline_));
  vkCmdBindVertexBuffers(types::get(current_command_buffer_), 0, 1,
                         &types::get(vertex_buffer), &vertex_offset);
  vkCmdBindIndexBuffer(types::get(current_command_buffer_),
                       types::get(index_buffer), index_offset,
                       VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(
      types::get(current_command_buffer_), VK_PIPELINE_BIND_POINT_GRAPHICS,
      types::get(pipeline_layout_), 0, 1,
      &types::get(descriptor_sets_[current_image_index_]), 0, nullptr);
  vkCmdDrawIndexed(types::get(current_command_buffer_),
                   static_cast<uint32_t>(std::size(indices_)), 1, 0, 0, 0);
}

void
renderer::end_draw()
{
  vkCmdEndRenderPass(types::get(current_command_buffer_));
  vkEndCommandBuffer(types::get(current_command_buffer_));
  // Wait for the image in flight to end if it is
  auto & image_in_flight_fence =
      image_in_flight_fences_[current_image_index_];

  if (image_in_flight_fence != nullptr)
  {
    vkWaitForFences(types::get(device_), 1,
                    &types::get(*image_in_flight_fence), VK_TRUE,
                    std::numeric_limits<int64_t>::max());
  }

  image_in_flight_fence = &frame_in_flight_fences_[current_frame_index_];

  // get current semaphores
  auto const image_available_semaphore =
      types::decay(image_available_semaphores_[current_frame_index_]);

  auto const render_finished_semaphore =
      types::decay(render_finished_semaphores_[current_frame_index_]);

  detail::submit_draw_commands(
      types::decay(device_), graphics_queue_,
      types::get(current_command_buffer_), image_available_semaphore,
      render_finished_semaphore, types::decay(*image_in_flight_fence));

  auto present_result = VK_ERROR_UNKNOWN;
  auto const check_present_result = [&present_result](auto const result)
  {
    present_result = result;
  };

  detail::present_swapchain(present_queue_, types::decay(swapchain_),
                            types::decay(render_finished_semaphore),
                            current_image_index_, check_present_result);

  auto const resized = window_.framebuffer_resized();
  auto const change_swapchain =
      (present_result == VK_ERROR_OUT_OF_DATE_KHR) ||
      (present_result == VK_SUBOPTIMAL_KHR);

  if (change_swapchain || resized)
  {
    window_.set_framebuffer_resized(false);
    recreate_after_framebuffer_change();
    return;
  }

  MVK_VERIFY(VK_SUCCESS == present_result);

  current_frame_index_ = (current_frame_index_ + 1) % max_frames_in_flight;
  vertex_buffer_manager_.next_frame();
  index_buffer_manager_.next_frame();
}

[[nodiscard]] float
renderer::time() const noexcept
{
  auto const current_time = std::chrono::high_resolution_clock::now();
  auto const delta_time = current_time - start_time;
  return std::chrono::duration<float, std::chrono::seconds::period>(
             delta_time)
      .count();
}

[[nodiscard]] pvm
renderer::create_test_pvm()
{
  auto const current_time = time();

  constexpr auto turn_rate = glm::radians(90.0F);

  auto ubo = pvm();

  ubo.model = glm::rotate(glm::mat4(1.0F), current_time * turn_rate,
                          glm::vec3(0.0F, 0.0F, 1.0F));
  ubo.view =
      glm::lookAt(glm::vec3(2.0F, 2.0F, 2.0F), glm::vec3(0.0F, 0.0F, 0.0F),
                  glm::vec3(0.0F, 0.0F, 1.0F));

  auto const ratio =
      static_cast<float>(extent_.width) / static_cast<float>(extent_.height);

  ubo.proj = glm::perspective(glm::radians(45.0F), ratio, 0.1F, 10.0F);
  ubo.proj[1][1] *= -1;

  return ubo;
}

} // namespace mvk
