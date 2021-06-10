#include "renderer.hpp"

#include "detail/creators.hpp"
#include "detail/misc.hpp"
#include "detail/readers.hpp"
#include "utility/misc.hpp"
#include "utility/slice.hpp"
#include "utility/verify.hpp"

#include <fstream>
#include <iostream>
#include <vector>

namespace mvk
{

void
renderer::init()
{
        window_ = vk_types::window({600, 600});

        init_vulkan();
        init_swapchain();
        preload_stuff();
        init_main_renderpass();
        init_framebuffers();
        init_commands();
        init_descriptors();
        init_pipeline();
        init_sync();
        load_mesh();
}

void
renderer::init_vulkan()
{
        MVK_VERIFY(vk_types::validation::check_support());

        auto application_info = []
        {
                auto info               = VkApplicationInfo();
                info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
                info.pApplicationName   = "stan loona";
                info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
                info.pEngineName        = "No Engine";
                info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
                return info;
        }();

        using vk_types::validation::validation_layers_data;
        auto const validation_layers   = validation_layers_data();
        auto const required_extensions = window_.required_extensions();

        auto instance_create_info = [validation_layers, &required_extensions, &application_info]
        {
                auto const [validation_data, validation_count] = utility::bind_data_and_size(validation_layers);
                auto const [required_data, required_count]     = utility::bind_data_and_size(required_extensions);

                using vk_types::validation::debug_create_info_ref;

                auto info                    = VkInstanceCreateInfo();
                info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
                info.pNext                   = debug_create_info_ref();
                info.pApplicationInfo        = &application_info;
                info.enabledLayerCount       = static_cast<uint32_t>(validation_count);
                info.ppEnabledLayerNames     = validation_data;
                info.enabledExtensionCount   = static_cast<uint32_t>(required_count);
                info.ppEnabledExtensionNames = required_data;
                return info;
        }();

        instance_        = vk_types::instance(instance_create_info);
        surface_         = vk_types::surface(instance_.get(), window_.get());
        debug_messenger_ = vk_types::debug_messenger(instance_.get());

        constexpr auto device_extensions    = std::array{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        auto const     physical_device      = detail::choose_physical_device(surface_.parent(), surface_.get(), device_extensions);
        auto const     queue_indices_result = detail::query_family_indices(physical_device, surface_.get());

        MVK_VERIFY(queue_indices_result.has_value());

        auto const queue_indices = queue_indices_result.value();

        auto features = VkPhysicalDeviceFeatures();
        vkGetPhysicalDeviceFeatures(physical_device, &features);

        auto queue_priority = 1.0F;

        auto const graphics_queue_create_info = [&queue_priority, &queue_indices]
        {
                auto const graphics_index = queue_indices.first;

                auto info             = VkDeviceQueueCreateInfo();
                info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                info.queueFamilyIndex = graphics_index;
                info.queueCount       = 1;
                info.pQueuePriorities = &queue_priority;
                return info;
        }();

        auto const present_queue_create_info = [&queue_priority, &queue_indices]
        {
                auto const present_index = queue_indices.second;

                auto info             = VkDeviceQueueCreateInfo();
                info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                info.queueFamilyIndex = present_index;
                info.queueCount       = 1;
                info.pQueuePriorities = &queue_priority;
                return info;
        }();

        auto const queue_create_info       = std::array{graphics_queue_create_info, present_queue_create_info};
        auto const queue_create_info_count = static_cast<uint32_t>(queue_indices.first != queue_indices.second ? 2 : 1);

        auto const device_create_info = [validation_layers, &queue_create_info, &device_extensions, queue_create_info_count, &features]
        {
                auto const [validation_data, validation_count] = utility::bind_data_and_size(validation_layers);
                auto const [extensions_data, extensions_count] = utility::bind_data_and_size(device_extensions);

                auto info                    = VkDeviceCreateInfo();
                info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
                info.queueCreateInfoCount    = queue_create_info_count;
                info.pQueueCreateInfos       = std::data(queue_create_info);
                info.pEnabledFeatures        = &features;
                info.enabledExtensionCount   = static_cast<uint32_t>(extensions_count);
                info.ppEnabledExtensionNames = extensions_data;
                info.enabledLayerCount       = static_cast<uint32_t>(validation_count);
                info.ppEnabledLayerNames     = validation_data;
                return info;
        }();

        device_ = vk_types::device(physical_device, device_create_info);

        auto const command_pool_create_info = [this]
        {
                auto const [graphics_queue, present_queue] = device_.get_queues();

                auto info             = VkCommandPoolCreateInfo();
                info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                info.queueFamilyIndex = graphics_queue.index();
                info.flags            = 0;
                return info;
        }();

        command_pool_ = vk_types::command_pool(device_.get(), command_pool_create_info);
}

void
renderer::init_swapchain()
{
        auto const [graphics_queue, present_queue] = device_.get_queues();
        auto const family_indices                  = std::array{graphics_queue.index(), present_queue.index()};

        auto const swapchain_create_info = [this, &family_indices]
        {
                auto const framebuffer_size = window_.query_framebuffer_size();
                auto const width            = static_cast<uint32_t>(framebuffer_size.width_);
                auto const height           = static_cast<uint32_t>(framebuffer_size.height_);
                auto const physical_device  = device_.physical_device();
                auto const format           = surface_.query_format(physical_device);
                auto const capabilities     = surface_.query_capabilities(physical_device);
                auto const present_mode     = detail::choose_present_mode(physical_device, surface_.get());
                auto const extent           = detail::choose_extent(capabilities, {width, height});
                auto const image_count      = detail::choose_image_count(capabilities);

                auto info             = VkSwapchainCreateInfoKHR();
                info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
                info.surface          = surface_.get();
                info.minImageCount    = image_count;
                info.imageFormat      = format.format;
                info.imageColorSpace  = format.colorSpace;
                info.imageExtent      = extent;
                info.imageArrayLayers = 1;
                info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                info.preTransform     = capabilities.currentTransform;
                info.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
                info.presentMode      = present_mode;
                info.clipped          = VK_TRUE;
                info.oldSwapchain     = nullptr;

                if (family_indices[0] != family_indices[1])
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

                return info;
        }();

        swapchain_ = vk_types::swapchain(device_.get(), swapchain_create_info);

        auto const depth_image_create_info = [this]
        {
                auto info          = VkImageCreateInfo();
                info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                info.imageType     = VK_IMAGE_TYPE_2D;
                info.extent.width  = swapchain_.extent().width;
                info.extent.height = swapchain_.extent().height;
                info.extent.depth  = 1;
                info.mipLevels     = 1;
                info.arrayLayers   = 1;
                info.format        = VK_FORMAT_D32_SFLOAT;
                info.tiling        = VK_IMAGE_TILING_OPTIMAL;
                info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                info.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
                info.samples       = VK_SAMPLE_COUNT_1_BIT;
                info.flags         = 0;
                return info;
        }();

        depth_image_        = vk_types::image(device_.get(), depth_image_create_info);
        depth_image_memory_ = detail::create_device_memory(device_, depth_image_, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        auto const depth_image_view_create_info = [this]
        {
                auto info                            = VkImageViewCreateInfo();
                info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                info.image                           = depth_image_.get();
                info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
                info.format                          = VK_FORMAT_D32_SFLOAT;
                info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
                info.subresourceRange.baseMipLevel   = 0;
                info.subresourceRange.levelCount     = 1;
                info.subresourceRange.baseArrayLayer = 0;
                info.subresourceRange.layerCount     = 1;
                return info;
        }();

        depth_image_view_ = vk_types::image_view(device_.get(), depth_image_view_create_info);
        depth_image_.transition_layout(device_, command_pool_, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void
renderer::preload_stuff()
{
        auto const vertex_code   = detail::read_file("../../shaders/vert.spv");
        auto       vertex_shader = detail::create_shader_module(device_, vertex_code);

        auto const fragment_code   = detail::read_file("../../shaders/frag.spv");
        auto       fragment_shader = detail::create_shader_module(device_, fragment_code);

        builder_.add_stage(std::move(vertex_shader), VK_SHADER_STAGE_VERTEX_BIT).add_stage(std::move(fragment_shader), VK_SHADER_STAGE_FRAGMENT_BIT);

        texture_ = vk_types::image::texture("../../assets/viking_room.png");

        auto const image_create_info = [this]
        {
                auto usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

                auto info          = VkImageCreateInfo();
                info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                info.imageType     = VK_IMAGE_TYPE_2D;
                info.extent.width  = texture_.width();
                info.extent.height = texture_.height();
                info.extent.depth  = 1;
                info.mipLevels     = texture_.mipmap_levels();
                info.arrayLayers   = 1;
                info.format        = VK_FORMAT_R8G8B8A8_SRGB;
                info.tiling        = VK_IMAGE_TILING_OPTIMAL;
                info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                info.usage         = static_cast<VkFlags>(usage);
                info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
                info.samples       = VK_SAMPLE_COUNT_1_BIT;
                info.flags         = 0;
                return info;
        }();

        image_        = vk_types::image(device_.get(), image_create_info);
        image_memory_ = detail::create_device_memory(device_, image_, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        image_.transition_layout(device_, command_pool_, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        image_.stage(device_, command_pool_, texture_);
        image_.generate_mipmaps(device_, command_pool_, texture_.width(), texture_.height());

        auto const image_view_create_info = [this]
        {
                auto info                            = VkImageViewCreateInfo();
                info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                info.image                           = image_.get();
                info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
                info.format                          = VK_FORMAT_R8G8B8A8_SRGB;
                info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                info.subresourceRange.baseMipLevel   = 0;
                info.subresourceRange.levelCount     = image_.mipmap_levels();
                info.subresourceRange.baseArrayLayer = 0;
                info.subresourceRange.layerCount     = 1;
                return info;
        }();

        image_view_ = vk_types::image_view(device_.get(), image_view_create_info);

        auto const sampler_create_info = [this]
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
                info.maxLod                  = static_cast<float>(image_.mipmap_levels());
                return info;
        }();

        sampler_ = vk_types::sampler(device_.get(), sampler_create_info);

        vertex_buffer_manager_ = buffer_manager(&device_, &command_pool_, buffer_type::vertex);
        index_buffer_manager_  = buffer_manager(&device_, &command_pool_, buffer_type::index);

        std::tie(vertices_, indices_) = detail::read_object("../../assets/viking_room.obj");
}

void
renderer::init_main_renderpass()
{
        auto const format = surface_.query_format(device_.physical_device()).format;

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
                attachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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

        auto const subpass_description = [&color_attachment_reference, &depth_attachment_reference]
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

        auto const render_pass_create_info = [&attachments, &subpass_description, &subpass_dependency]
        {
                auto const [attachments_data, attachments_size] = utility::bind_data_and_size(attachments);

                auto info            = VkRenderPassCreateInfo();
                info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                info.attachmentCount = static_cast<uint32_t>(attachments_size);
                info.pAttachments    = attachments_data;
                info.subpassCount    = 1;
                info.pSubpasses      = &subpass_description;
                info.dependencyCount = 1;
                info.pDependencies   = &subpass_dependency;
                return info;
        }();

        render_pass_ = vk_types::render_pass(device_.get(), render_pass_create_info);
}

void
renderer::init_framebuffers()
{

        auto const image_views = swapchain_.image_views();
        framebuffers_.reserve(std::size(image_views));

        auto const add_framebuffer = [this](auto const & image_view)
        {
                auto const attachments = std::array{image_view.get(), depth_image_view_.get()};

                auto const framebuffer_create_info = [this, &attachments]
                {
                        auto const extent                               = swapchain_.extent();
                        auto const [attachments_data, attachments_size] = utility::bind_data_and_size(attachments);

                        auto info            = VkFramebufferCreateInfo();
                        info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                        info.renderPass      = render_pass_.get();
                        info.attachmentCount = static_cast<uint32_t>(attachments_size);
                        info.pAttachments    = attachments_data;
                        info.width           = extent.width;
                        info.height          = extent.height;
                        info.layers          = 1;
                        return info;
                }();

                framebuffers_.emplace_back(device_.get(), framebuffer_create_info);
        };
        std::for_each(std::begin(image_views), std::end(image_views), add_framebuffer);
}

void
renderer::init_commands()
{
        auto const command_buffers_allocate_info = [this]
        {
                auto info               = VkCommandBufferAllocateInfo();
                info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                info.commandPool        = command_pool_.get();
                info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                info.commandBufferCount = static_cast<uint32_t>(std::size(framebuffers_));
                return info;
        }();

        command_buffers_ = vk_types::command_buffers(device_.get(), command_buffers_allocate_info);
}

void
renderer::init_descriptors()
{
        auto const descriptor_set_layout_bindings = std::array{[]
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

        auto const descriptor_set_layout_create_info = [&descriptor_set_layout_bindings]
        {
                auto const [descriptor_set_binding_data, descriptor_set_binding_size] = utility::bind_data_and_size(descriptor_set_layout_bindings);

                auto info         = VkDescriptorSetLayoutCreateInfo();
                info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                info.bindingCount = static_cast<uint32_t>(descriptor_set_binding_size);
                info.pBindings    = descriptor_set_binding_data;
                return info;
        }();

        descriptor_set_layout_ = vk_types::descriptor_set_layout(device_.get(), descriptor_set_layout_create_info);

        auto const images_count = static_cast<uint32_t>(std::size(swapchain_.images()));

        auto const descriptor_pool_sizes = std::array{[images_count]
                                                      {
                                                              auto uniform_pool_size            = VkDescriptorPoolSize();
                                                              uniform_pool_size.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                                                              uniform_pool_size.descriptorCount = images_count;
                                                              return uniform_pool_size;
                                                      }(),
                                                      [images_count]
                                                      {
                                                              auto sampler_pool_size            = VkDescriptorPoolSize();
                                                              sampler_pool_size.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                                                              sampler_pool_size.descriptorCount = images_count;
                                                              return sampler_pool_size;
                                                      }()};

        auto const descriptor_pool_create_info = [&descriptor_pool_sizes, images_count]
        {
                auto const [descriptor_pool_sizes_data, descriptor_pool_sizes_count] = utility::bind_data_and_size(descriptor_pool_sizes);

                auto info          = VkDescriptorPoolCreateInfo();
                info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                info.poolSizeCount = static_cast<uint32_t>(descriptor_pool_sizes_count);
                info.pPoolSizes    = descriptor_pool_sizes_data;
                info.maxSets       = images_count;
                info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                return info;
        }();

        descriptor_pool_ = vk_types::descriptor_pool(device_.get(), descriptor_pool_create_info);

        auto const images_size = std::size(swapchain_.images());

        auto descriptor_set_layouts = std::vector(images_size, descriptor_set_layout_.get());

        auto const descriptor_sets_allocate_info = [this, &descriptor_set_layouts]
        {
                auto const [descriptor_set_layouts_data, descriptor_set_layouts_size] = utility::bind_data_and_size(descriptor_set_layouts);

                auto info               = VkDescriptorSetAllocateInfo();
                info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                info.descriptorPool     = descriptor_pool_.get();
                info.descriptorSetCount = static_cast<uint32_t>(descriptor_set_layouts_size);
                info.pSetLayouts        = descriptor_set_layouts_data;
                return info;
        }();

        descriptor_sets_ = vk_types::descriptor_sets(device_.get(), descriptor_sets_allocate_info);

        uniform_buffers_.reserve(images_size);
        uniform_buffers_memory_.reserve(images_size);

        for (auto i = size_t(0); i < images_size; ++i)
        {
                auto const uniform_buffer_create_info = []
                {
                        auto info        = VkBufferCreateInfo();
                        info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                        info.size        = sizeof(pvm);
                        info.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                        return info;
                }();

                uniform_buffers_.emplace_back(device_.get(), uniform_buffer_create_info);

                auto uniform_buffer_memory = detail::create_device_memory(device_, uniform_buffers_.back(),
                                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                uniform_buffers_memory_.push_back(std::move(uniform_buffer_memory));

                uniform_buffers_memory_.back().map(sizeof(pvm));
        }

        for (auto i = size_t(0); i < images_size; ++i)
        {
                auto descriptor_buffer_info   = VkDescriptorBufferInfo();
                descriptor_buffer_info.buffer = uniform_buffers_[i].get();
                descriptor_buffer_info.offset = 0;
                descriptor_buffer_info.range  = sizeof(pvm);

                auto descriptor_image_info        = VkDescriptorImageInfo();
                descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                descriptor_image_info.imageView   = image_view_.get();
                descriptor_image_info.sampler     = sampler_.get();

                auto const descriptor_writes = std::array{[this, &descriptor_buffer_info, i]
                                                          {
                                                                  auto ubo             = VkWriteDescriptorSet();
                                                                  ubo.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                                                  ubo.dstSet           = descriptor_sets_.get()[i];
                                                                  ubo.dstBinding       = 0;
                                                                  ubo.dstArrayElement  = 0;
                                                                  ubo.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                                                                  ubo.descriptorCount  = 1;
                                                                  ubo.pBufferInfo      = &descriptor_buffer_info;
                                                                  ubo.pImageInfo       = nullptr;
                                                                  ubo.pTexelBufferView = nullptr;
                                                                  return ubo;
                                                          }(),
                                                          [this, &descriptor_image_info, i]
                                                          {
                                                                  auto image            = VkWriteDescriptorSet();
                                                                  image.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                                                  image.dstSet          = descriptor_sets_.get()[i];
                                                                  image.dstBinding      = 1;
                                                                  image.dstArrayElement = 0;
                                                                  image.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                                                                  image.descriptorCount = 1;
                                                                  image.pImageInfo      = &descriptor_image_info;
                                                                  return image;
                                                          }()};

                vk_types::update_descriptor_sets(device_.get(), descriptor_writes, {});
        }
}

void
renderer::init_pipeline()
{
        auto const descriptor_set_layout = descriptor_set_layout_.get();

        auto const pipeline_layout_create_info = [&descriptor_set_layout]
        {
                auto info                   = VkPipelineLayoutCreateInfo();
                info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                info.setLayoutCount         = 1;
                info.pSetLayouts            = &descriptor_set_layout;
                info.pushConstantRangeCount = 0;
                info.pPushConstantRanges    = nullptr;
                return info;
        }();

        pipeline_layout_ = vk_types::pipeline_layout(device_.get(), pipeline_layout_create_info);

        auto const vertex_input_binding_description = []
        {
                auto description      = VkVertexInputBindingDescription();
                description.binding   = 0;
                description.stride    = sizeof(vertex);
                description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                return description;
        }();

        auto const vertex_attributes = std::array{[]
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

        auto const vertex_input_create_info = [&vertex_input_binding_description, &vertex_attributes]
        {
                auto const [vertex_attributes_data, vertex_attributes_size] = utility::bind_data_and_size(vertex_attributes);

                auto info                            = VkPipelineVertexInputStateCreateInfo();
                info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                info.vertexBindingDescriptionCount   = 1;
                info.pVertexBindingDescriptions      = &vertex_input_binding_description;
                info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attributes_size);
                info.pVertexAttributeDescriptions    = vertex_attributes_data;
                return info;
        }();

        auto const input_assembly_create_info = []
        {
                auto info                   = VkPipelineInputAssemblyStateCreateInfo();
                info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
                info.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                info.primitiveRestartEnable = VK_FALSE;
                return info;
        }();

        auto const extent = swapchain_.extent();

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
                auto info                    = VkPipelineRasterizationStateCreateInfo();
                info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
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
                auto info                  = VkPipelineMultisampleStateCreateInfo();
                info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
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
                auto attachment                = VkPipelineColorBlendAttachmentState();
                attachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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
                auto info              = VkPipelineColorBlendStateCreateInfo();
                info.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
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
                auto info                  = VkPipelineDepthStencilStateCreateInfo();
                info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                info.depthTestEnable       = VK_TRUE;
                info.depthWriteEnable      = VK_TRUE;
                info.depthCompareOp        = VK_COMPARE_OP_LESS;
                info.depthBoundsTestEnable = VK_FALSE;
                info.minDepthBounds        = 0.0F;
                info.maxDepthBounds        = 1.0F;
                info.stencilTestEnable     = VK_FALSE;
                return info;
        }();

        auto const pipeline_create_info = [&, this]
        {
                auto const shader_stages                            = builder_.stages();
                auto const [shader_stages_data, shader_stages_size] = utility::bind_data_and_size(shader_stages);

                auto info                = VkGraphicsPipelineCreateInfo();
                info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                info.stageCount          = static_cast<uint32_t>(shader_stages_size);
                info.pStages             = shader_stages_data;
                info.pVertexInputState   = &vertex_input_create_info;
                info.pInputAssemblyState = &input_assembly_create_info;
                info.pViewportState      = &viewport_state_create_info;
                info.pRasterizationState = &rasterizer_create_info;
                info.pMultisampleState   = &multisampling_create_info;
                info.pDepthStencilState  = &depth_stencil_state_create_info;
                info.pColorBlendState    = &color_blend_create_info;
                info.pDynamicState       = nullptr;
                info.layout              = pipeline_layout_.get();
                info.renderPass          = render_pass_.get();
                info.subpass             = 0;
                info.basePipelineHandle  = nullptr;
                info.basePipelineIndex   = -1;
                return info;
        }();

        pipeline_ = vk_types::pipeline(device_.get(), pipeline_create_info);
}
void
renderer::init_sync()
{
        auto const semaphore_create_info = []
        {
                auto info  = VkSemaphoreCreateInfo();
                info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                return info;
        }();

        auto const fence_create_info = []
        {
                auto info  = VkFenceCreateInfo();
                info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
                return info;
        }();

        for (auto i = size_t(0); i < max_frames_in_flight; ++i)
        {
                image_available_semaphores_.at(i) = vk_types::semaphore(device_.get(), semaphore_create_info);
                render_finished_semaphores_.at(i) = vk_types::semaphore(device_.get(), semaphore_create_info);
                frame_in_flight_fences_.at(i)     = vk_types::fence(device_.get(), fence_create_info);
        }

        image_in_flight_fences_.resize(std::size(swapchain_.images()), nullptr);
}

void
renderer::load_mesh()
{
        auto const [vertex_buffer, vertex_offset] = vertex_buffer_manager_.map(utility::as_bytes(vertices_));
        auto const [index_buffer, index_offset]   = index_buffer_manager_.map(utility::as_bytes(indices_));

        for (size_t i = 0; i < std::size(command_buffers_.get()); ++i)
        {
                auto clear_color_value  = VkClearValue();
                clear_color_value.color = {{0.0F, 0.0F, 0.0F, 1.0F}};

                auto clear_depth_value         = VkClearValue();
                clear_depth_value.depthStencil = {1.0F, 0};

                auto const clear_values = std::array{clear_color_value, clear_depth_value};

                auto const clear_values_size = static_cast<uint32_t>(std::size(clear_values));

                auto render_pass_begin_info                = VkRenderPassBeginInfo();
                render_pass_begin_info.sType               = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                render_pass_begin_info.renderPass          = render_pass_.get();
                render_pass_begin_info.framebuffer         = framebuffers_[i].get();
                render_pass_begin_info.renderArea.offset.x = 0;
                render_pass_begin_info.renderArea.offset.y = 0;
                render_pass_begin_info.renderArea.extent   = swapchain_.extent();
                render_pass_begin_info.clearValueCount     = clear_values_size;
                render_pass_begin_info.pClearValues        = std::data(clear_values);

                auto command_buffer_begin_info             = VkCommandBufferBeginInfo();
                command_buffer_begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                command_buffer_begin_info.flags            = 0;
                command_buffer_begin_info.pInheritanceInfo = nullptr;

                command_buffers_.begin(i, command_buffer_begin_info)
                        .begin_render_pass(render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE)
                        .bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.get())
                        .bind_vertex_buffer(vertex_buffer.get(), {&vertex_offset, 1})
                        .bind_index_buffer(index_buffer.get(), index_offset)
                        .bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_.get(), 0, 1, descriptor_sets_.get()[i])
                        .draw_indexed(static_cast<uint32_t>(std::size(indices_)), 1, 0, 0, 0)
                        .end_render_pass()
                        .end();
        }
}

void
renderer::recreate_after_framebuffer_change()
{
        init_swapchain();
        init_main_renderpass();

        framebuffers_.clear();

        init_framebuffers();
        init_commands();

        uniform_buffers_.clear();
        uniform_buffers_memory_.clear();

        descriptor_sets_.release();

        init_descriptors();
        init_pipeline();
        init_sync();
        load_mesh();
}

void
renderer::run()
{
        auto current_frame = size_t(0);

        while (!window_.should_close())
        {
                glfwPollEvents();

                auto const & image_available_semaphore = image_available_semaphores_[current_frame];

                auto const & image_index = swapchain_.next_image(image_available_semaphore.get());

                if (!image_index.has_value())
                {
                        recreate_after_framebuffer_change();
                        continue;
                }

                auto const current_index         = image_index.value();
                auto &     frame_in_flight_fence = frame_in_flight_fences_[current_frame];
                auto &     image_in_flight_fence = image_in_flight_fences_[current_index];

                if (image_in_flight_fence != nullptr)
                {
                        image_in_flight_fence->wait();
                }

                image_in_flight_fence = &frame_in_flight_fence;

                auto const render_finished_semaphore = render_finished_semaphores_[current_frame].get();

                auto const wait_semaphores   = std::array{image_available_semaphore.get()};
                auto const signal_semaphores = std::array{render_finished_semaphore};
                auto const wait_stages       = std::array<VkPipelineStageFlags, 1>{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

                auto const swapchains   = std::array{swapchain_.get()};
                auto const current_time = std::chrono::high_resolution_clock::now();
                auto const delta_time   = current_time - start_time;
                auto const time         = std::chrono::duration<float, std::chrono::seconds::period>(delta_time).count();

                constexpr auto turn_rate = glm::radians(90.0F);

                auto ubo = pvm();

                ubo.model = glm::rotate(glm::mat4(1.0F), time * turn_rate, glm::vec3(0.0F, 0.0F, 1.0F));

                ubo.view = glm::lookAt(glm::vec3(2.0F, 2.0F, 2.0F), glm::vec3(0.0F, 0.0F, 0.0F), glm::vec3(0.0F, 0.0F, 1.0F));

                auto const ratio = static_cast<float>(swapchain_.extent().width) / static_cast<float>(swapchain_.extent().height);

                ubo.proj = glm::perspective(glm::radians(45.0F), ratio, 0.1F, 10.0F);

                ubo.proj[1][1] *= -1;

                auto & current_ubo_memory = uniform_buffers_memory_[current_index];

                current_ubo_memory.copy_data({utility::force_cast_to_byte(&ubo), sizeof(ubo)});

                auto const submit_info = [this, &wait_semaphores, &signal_semaphores, &wait_stages, current_index]
                {
                        auto const wait_semaphore_count = static_cast<uint32_t>(std::size(wait_semaphores));

                        auto const signal_semaphore_count = static_cast<uint32_t>(std::size(signal_semaphores));

                        auto const command_buffers_ptr = &command_buffers_.get()[current_index];

                        auto info                 = VkSubmitInfo();
                        info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                        info.waitSemaphoreCount   = wait_semaphore_count;
                        info.pWaitSemaphores      = std::data(wait_semaphores);
                        info.pWaitDstStageMask    = std::data(wait_stages);
                        info.commandBufferCount   = 1;
                        info.pCommandBuffers      = command_buffers_ptr;
                        info.signalSemaphoreCount = signal_semaphore_count;
                        info.pSignalSemaphores    = std::data(signal_semaphores);
                        return info;
                }();

                frame_in_flight_fence.reset();

                auto [graphics_queue, present_queue] = device_.get_queues();
                graphics_queue.submit(submit_info, frame_in_flight_fence);

                auto const present_info = [&signal_semaphores, &swapchains, &current_index]
                {
                        auto const wait_semaphore_count = static_cast<uint32_t>(std::size(signal_semaphores));

                        auto const swapchain_count = static_cast<uint32_t>(std::size(swapchains));

                        auto info               = VkPresentInfoKHR();
                        info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                        info.waitSemaphoreCount = wait_semaphore_count;
                        info.pWaitSemaphores    = std::data(signal_semaphores);
                        info.swapchainCount     = swapchain_count;
                        info.pSwapchains        = std::data(swapchains);
                        info.pImageIndices      = &current_index;
                        info.pResults           = nullptr;
                        return info;
                }();

                auto       present_result       = VK_ERROR_UNKNOWN;
                auto const check_present_result = [&present_result](auto const result)
                {
                        present_result = result;
                };

                present_queue.present(present_info, check_present_result).wait_idle();

                auto const resized          = window_.framebuffer_resized();
                auto const change_swapchain = (present_result == VK_ERROR_OUT_OF_DATE_KHR) || (present_result == VK_SUBOPTIMAL_KHR);

                if (change_swapchain || resized)
                {
                        window_.set_framebuffer_resized(false);
                        recreate_after_framebuffer_change();
                        continue;
                }

                MVK_VERIFY(VK_SUCCESS == present_result);

                current_frame = (current_frame + 1) % max_frames_in_flight;
        }

        device_.wait_idle();
}

} // namespace mvk
