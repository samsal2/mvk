#include "context.hpp"

#include "detail/helpers.hpp"
#include "detail/misc.hpp"
#include "detail/readers.hpp"
#include "types/types.hpp"
#include "utility/slice.hpp"
#include "utility/verify.hpp"
#include "vulkan/vulkan_core.h"

#include <limits>

namespace mvk
{
  [[nodiscard]] context create_context( std::string const & name, types::window::extent extent ) noexcept
  {
    auto ctx = context();
    init_window( ctx, extent );
    init_instance( ctx, name );
    init_debug_messenger( ctx );
    init_surface( ctx );
    select_physical_device( ctx );
    select_surface_format( ctx );
    init_device( ctx );
    create_staging_buffers_and_memories( ctx, 1024 * 1024 );
    init_layouts( ctx );
    init_pools( ctx );
    init_swapchain( ctx );
    init_depth_image( ctx );
    init_main_render_pass( ctx );
    init_framebuffers( ctx );
    init_doesnt_belong_here( ctx );
    allocate_command_buffers( ctx );
    init_shaders( ctx );
    init_pipeline( ctx );
    init_sync( ctx );
    create_vertex_buffers_and_memories( ctx, 1024 * 1024 );
    create_index_buffers_and_memories( ctx, 1024 * 1024 );
    create_uniform_buffers_memories_and_sets( ctx, 1024 * 1024 );
    return ctx;
  }

  void init_window( context & ctx, types::window::extent extent ) noexcept
  {
    ctx.window_ = types::window( extent );
  }

  void init_instance( context & ctx, std::string const & name ) noexcept
  {
    if constexpr ( context::use_validation )
    {
      auto validation_layer_properties_count = uint32_t( 0 );
      vkEnumerateInstanceLayerProperties( &validation_layer_properties_count, nullptr );

      auto validation_layer_properties = std::vector<VkLayerProperties>( validation_layer_properties_count );
      vkEnumerateInstanceLayerProperties( &validation_layer_properties_count,
                                          std::data( validation_layer_properties ) );

      auto const is_layer_present =
        []( VkLayerProperties const layer, utility::slice<char const * const> const layers ) noexcept
      {
        auto const matches = [&layer]( auto const & current_layer )
        {
          auto const layer_name_data = std::data( layer.layerName );
          return std::strcmp( current_layer, layer_name_data ) == 0;
        };

        return std::any_of( std::begin( layers ), std::end( layers ), matches );
      };

      auto const exists = [is_layer_present]( auto const & available_layer )
      {
        return is_layer_present( available_layer, context::validation_layers );
      };

      auto result =
        std::any_of( std::begin( validation_layer_properties ), std::end( validation_layer_properties ), exists );
      MVK_VERIFY( result );
    }

    auto application_info = [&name]
    {
      auto info               = VkApplicationInfo();
      info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      info.pApplicationName   = name.c_str();
      info.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
      info.pEngineName        = "No Engine";
      info.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );
      return info;
    }();

    auto const required_extensions  = ctx.window_.required_extensions();
    auto const instance_create_info = [&required_extensions, &application_info]
    {
      auto info  = VkInstanceCreateInfo();
      info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

      if constexpr ( context::use_validation )
      {
        info.pNext = &detail::debug_create_info;
      }
      else
      {
        info.pNext = nullptr;
      }

      info.pApplicationInfo = &application_info;

      if constexpr ( context::use_validation )
      {
        info.enabledLayerCount   = static_cast<uint32_t>( std::size( context::validation_layers ) );
        info.ppEnabledLayerNames = std::data( context::validation_layers );
      }
      else
      {
        info.enabledLayerCount       = 0;
        info.ppEnabledExtensionNames = nullptr;
      }

      info.enabledExtensionCount   = static_cast<uint32_t>( std::size( required_extensions ) );
      info.ppEnabledExtensionNames = std::data( required_extensions );
      return info;
    }();

    ctx.instance_ = types::create_unique_instance( instance_create_info );
  }

  void init_debug_messenger( context & ctx ) noexcept
  {
    ctx.debug_messenger_ = types::create_unique_debug_messenger( types::get( ctx.instance_ ) );
  }

  void init_surface( context & ctx ) noexcept
  {
    auto surface = types::surface();
    glfwCreateWindowSurface( types::get( ctx.instance_ ), types::get( ctx.window_ ), nullptr, &types::get( surface ) );
    ctx.surface_ = types::unique_surface( types::get( surface ), types::get( ctx.instance_ ) );
  }

  void select_physical_device( context & ctx ) noexcept
  {
    auto physical_device_count = uint32_t( 0 );
    vkEnumeratePhysicalDevices( types::get( ctx.instance_ ), &physical_device_count, nullptr );

    auto physical_devices = std::vector<VkPhysicalDevice>( physical_device_count );
    vkEnumeratePhysicalDevices( types::get( ctx.instance_ ), &physical_device_count, std::data( physical_devices ) );

    auto const supported = [&ctx]( auto const physical_device )
    {
      auto features = VkPhysicalDeviceFeatures();
      vkGetPhysicalDeviceFeatures( physical_device, &features );

      auto const extensions_supported = detail::check_extension_support( physical_device, context::device_extensions );
      auto const format_and_present_mode_available =
        detail::check_format_and_present_mode_availability( physical_device, types::decay( ctx.surface_ ) );
      auto const family_indices_found =
        detail::query_family_indices( physical_device, types::decay( ctx.surface_ ) ).has_value();

      return extensions_supported && format_and_present_mode_available && family_indices_found &&
             features.samplerAnisotropy;
    };

    auto const it = std::find_if( std::begin( physical_devices ), std::end( physical_devices ), supported );

    MVK_VERIFY( it != std::end( physical_devices ) );

    ctx.physical_device_ = *it;
  }

  void select_surface_format( context & ctx ) noexcept
  {
    auto formats_count = uint32_t( 0 );
    vkGetPhysicalDeviceSurfaceFormatsKHR(
      types::get( ctx.physical_device_ ), types::get( ctx.surface_ ), &formats_count, nullptr );

    auto formats = std::vector<VkSurfaceFormatKHR>( formats_count );
    vkGetPhysicalDeviceSurfaceFormatsKHR(
      types::get( ctx.physical_device_ ), types::get( ctx.surface_ ), &formats_count, std::data( formats ) );

    auto const meets_requirements = []( auto const & format )
    {
      return ( format.format == VK_FORMAT_B8G8R8A8_SRGB ) && ( format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR );
    };

    auto const it = std::find_if( std::begin( formats ), std::end( formats ), meets_requirements );

    ctx.surface_format_ = ( it != std::end( formats ) ) ? *it : formats[0];
  }

  void init_device( context & ctx ) noexcept
  {
    auto const queue_indices_result =
      detail::query_family_indices( ctx.physical_device_, types::decay( ctx.surface_ ) );

    MVK_VERIFY( queue_indices_result.has_value() );

    auto const queue_indices  = queue_indices_result.value();
    ctx.graphics_queue_index_ = queue_indices.first;
    ctx.present_queue_index_  = queue_indices.second;

    auto features = VkPhysicalDeviceFeatures();
    vkGetPhysicalDeviceFeatures( types::get( ctx.physical_device_ ), &features );

    auto const queue_priority = 1.0F;

    auto const graphics_queue_create_info = [&queue_priority, &ctx]
    {
      auto info             = VkDeviceQueueCreateInfo();
      info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      info.queueFamilyIndex = ctx.graphics_queue_index_;
      info.queueCount       = 1;
      info.pQueuePriorities = &queue_priority;
      return info;
    }();

    auto const present_queue_create_info = [&queue_priority, &ctx]
    {
      auto info             = VkDeviceQueueCreateInfo();
      info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      info.queueFamilyIndex = ctx.present_queue_index_;
      info.queueCount       = 1;
      info.pQueuePriorities = &queue_priority;
      return info;
    }();

    auto const queue_create_info       = std::array{ graphics_queue_create_info, present_queue_create_info };
    auto const queue_create_info_count = static_cast<uint32_t>( queue_indices.first != queue_indices.second ? 2 : 1 );

    auto const device_create_info = [&queue_create_info, queue_create_info_count, &features]
    {
      auto info                    = VkDeviceCreateInfo();
      info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      info.queueCreateInfoCount    = queue_create_info_count;
      info.pQueueCreateInfos       = std::data( queue_create_info );
      info.pEnabledFeatures        = &features;
      info.enabledExtensionCount   = static_cast<uint32_t>( std::size( context::device_extensions ) );
      info.ppEnabledExtensionNames = std::data( context::device_extensions );

      if constexpr ( context::use_validation )
      {
        info.enabledLayerCount   = static_cast<uint32_t>( std::size( context::validation_layers ) );
        info.ppEnabledLayerNames = std::data( context::validation_layers );
      }
      else
      {
        info.enabledLayerCount   = 0;
        info.ppEnabledLayerNames = nullptr;
      }

      return info;
    }();

    ctx.device_         = types::create_unique_device( types::decay( ctx.physical_device_ ), device_create_info );
    ctx.graphics_queue_ = types::get_queue( types::decay( ctx.device_ ), ctx.graphics_queue_index_ );
    ctx.present_queue_  = types::get_queue( types::decay( ctx.device_ ), ctx.present_queue_index_ );
  }

  void init_layouts( context & ctx ) noexcept
  {
    auto const uniform_layout = []
    {
      auto layout               = VkDescriptorSetLayoutBinding();
      layout.binding            = 0;
      layout.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
      layout.descriptorCount    = 1;
      layout.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
      layout.pImmutableSamplers = nullptr;
      return layout;
    }();

    auto const uniform_descriptor_set_bindings = std::array{ uniform_layout };

    auto const uniform_descriptor_set_layout_create_info = [&uniform_descriptor_set_bindings]
    {
      auto info         = VkDescriptorSetLayoutCreateInfo();
      info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      info.bindingCount = static_cast<uint32_t>( std::size( uniform_descriptor_set_bindings ) );
      info.pBindings    = std::data( uniform_descriptor_set_bindings );
      return info;
    }();

    ctx.uniform_descriptor_set_layout_ = types::create_unique_descriptor_set_layout(
      types::get( ctx.device_ ), uniform_descriptor_set_layout_create_info );

    auto const sampler_layout = []
    {
      auto layout               = VkDescriptorSetLayoutBinding();
      layout.binding            = 0;
      layout.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      layout.descriptorCount    = 1;
      layout.pImmutableSamplers = nullptr;
      layout.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
      return layout;
    }();

    auto const sampler_descriptor_set_bindings = std::array{ sampler_layout };

    auto const sampler_descriptor_set_layout_create_info = [&sampler_descriptor_set_bindings]
    {
      auto info         = VkDescriptorSetLayoutCreateInfo();
      info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      info.bindingCount = static_cast<uint32_t>( std::size( sampler_descriptor_set_bindings ) );
      info.pBindings    = std::data( sampler_descriptor_set_bindings );
      return info;
    }();

    ctx.texture_descriptor_set_layout_ = types::create_unique_descriptor_set_layout(
      types::get( ctx.device_ ), sampler_descriptor_set_layout_create_info );

    auto descriptor_set_layouts =
      std::array{ types::get( ctx.uniform_descriptor_set_layout_ ), types::get( ctx.texture_descriptor_set_layout_ ) };

    auto const pipeline_layout_create_info = [&descriptor_set_layouts]
    {
      auto info                   = VkPipelineLayoutCreateInfo();
      info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
      info.setLayoutCount         = static_cast<uint32_t>( std::size( descriptor_set_layouts ) );
      info.pSetLayouts            = std::data( descriptor_set_layouts );
      info.pushConstantRangeCount = 0;
      info.pPushConstantRanges    = nullptr;
      return info;
    }();

    ctx.pipeline_layout_ =
      types::create_unique_pipeline_layout( types::get( ctx.device_ ), pipeline_layout_create_info );
  }

  void init_pools( context & ctx ) noexcept
  {
    auto const command_pool_create_info = [&ctx]
    {
      auto info             = VkCommandPoolCreateInfo();
      info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      info.queueFamilyIndex = ctx.graphics_queue_index_;
      info.flags            = 0;
      return info;
    }();

    ctx.command_pool_ = types::create_unique_command_pool( types::get( ctx.device_ ), command_pool_create_info );

    auto const uniform_pool_size = []
    {
      auto pool_size            = VkDescriptorPoolSize();
      pool_size.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
      pool_size.descriptorCount = 32;
      return pool_size;
    }();

    auto const sampler_pool_size = []
    {
      auto pool_size            = VkDescriptorPoolSize();
      pool_size.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      pool_size.descriptorCount = 32;
      return pool_size;
    }();

    auto const descriptor_pool_sizes = std::array{ uniform_pool_size, sampler_pool_size };

    auto const descriptor_pool_create_info = [&descriptor_pool_sizes]
    {
      auto info          = VkDescriptorPoolCreateInfo();
      info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      info.poolSizeCount = static_cast<uint32_t>( std::size( descriptor_pool_sizes ) );
      info.pPoolSizes    = std::data( descriptor_pool_sizes );
      info.maxSets       = 128;
      info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
      return info;
    }();

    ctx.descriptor_pool_ =
      types::create_unique_descriptor_pool( types::decay( ctx.device_ ), descriptor_pool_create_info );
  }

  void init_swapchain( context & ctx ) noexcept
  {
    auto const family_indices = std::array{ ctx.graphics_queue_index_, ctx.present_queue_index_ };

    auto const swapchain_create_info = [&ctx, &family_indices]
    {
      auto const framebuffer_size = ctx.window_.query_framebuffer_size();
      auto const width            = static_cast<uint32_t>( framebuffer_size.width_ );
      auto const height           = static_cast<uint32_t>( framebuffer_size.height_ );

      auto capabilities = VkSurfaceCapabilitiesKHR();
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        types::get( ctx.physical_device_ ), types::get( ctx.surface_ ), &capabilities );

      auto const present_mode = detail::choose_present_mode( ctx.physical_device_, types::decay( ctx.surface_ ) );

      ctx.swapchain_extent_ = detail::choose_extent( capabilities, { width, height } );

      auto const image_count = detail::choose_image_count( capabilities );

      auto info             = VkSwapchainCreateInfoKHR();
      info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      info.surface          = types::get( ctx.surface_ );
      info.minImageCount    = image_count;
      info.imageFormat      = ctx.surface_format_.format;
      info.imageColorSpace  = ctx.surface_format_.colorSpace;
      info.imageExtent      = ctx.swapchain_extent_;
      info.imageArrayLayers = 1;
      info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      info.preTransform     = capabilities.currentTransform;
      info.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
      info.presentMode      = present_mode;
      info.clipped          = VK_TRUE;
      info.oldSwapchain     = nullptr;

      if ( family_indices[0] != family_indices[1] )
      {
        info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = 2;
        info.pQueueFamilyIndices   = std::data( family_indices );
      }
      else
      {
        info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        info.queueFamilyIndexCount = 0;
        info.pQueueFamilyIndices   = nullptr;
      }

      return info;
    }();

    ctx.swapchain_ = types::create_unique_swapchain( types::get( ctx.device_ ), swapchain_create_info );

    vkGetSwapchainImagesKHR(
      types::get( ctx.device_ ), types::get( ctx.swapchain_ ), &ctx.swapchain_images_count_, nullptr );

    auto swapchain_images = std::vector<VkImage>( ctx.swapchain_images_count_ );

    vkGetSwapchainImagesKHR( types::get( ctx.device_ ),
                             types::get( ctx.swapchain_ ),
                             &ctx.swapchain_images_count_,
                             std::data( swapchain_images ) );

    ctx.swapchain_image_views_.reserve( ctx.swapchain_images_count_ );

    auto const add_image_view = [&ctx, &swapchain_create_info]( auto const image )
    {
      auto const image_view_create_info = [image, &swapchain_create_info]
      {
        auto view_info                            = VkImageViewCreateInfo();
        view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image                           = image;
        view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format                          = swapchain_create_info.imageFormat;
        view_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel   = 0;
        view_info.subresourceRange.levelCount     = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount     = 1;
        return view_info;
      }();

      ctx.swapchain_image_views_.push_back(
        types::create_unique_image_view( types::decay( ctx.device_ ), image_view_create_info ) );
    };

    std::for_each( std::begin( swapchain_images ), std::end( swapchain_images ), add_image_view );
  }

  void init_depth_image( context & ctx ) noexcept
  {
    auto const depth_image_create_info = [&ctx]
    {
      auto info          = VkImageCreateInfo();
      info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      info.imageType     = VK_IMAGE_TYPE_2D;
      info.extent.width  = ctx.swapchain_extent_.width;
      info.extent.height = ctx.swapchain_extent_.height;
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

    ctx.depth_image_ = types::create_unique_image( types::get( ctx.device_ ), depth_image_create_info );

    auto depth_image_requirements = VkMemoryRequirements();
    vkGetImageMemoryRequirements(
      types::get( ctx.device_ ), types::get( ctx.depth_image_ ), &depth_image_requirements );

    auto const memory_type_index = detail::find_memory_type( types::get( ctx.physical_device_ ),
                                                             depth_image_requirements.memoryTypeBits,
                                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
    MVK_VERIFY( memory_type_index.has_value() );

    auto depth_image_memory_allocate_info = [depth_image_requirements, memory_type_index]
    {
      auto info            = VkMemoryAllocateInfo();
      info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      info.allocationSize  = depth_image_requirements.size;
      info.memoryTypeIndex = memory_type_index.value();
      return info;
    }();

    ctx.depth_image_memory_ =
      types::create_unique_device_memory( types::get( ctx.device_ ), depth_image_memory_allocate_info );

    vkBindImageMemory(
      types::get( ctx.device_ ), types::get( ctx.depth_image_ ), types::get( ctx.depth_image_memory_ ), 0 );

    auto const depth_image_view_create_info = [&ctx]
    {
      auto info                            = VkImageViewCreateInfo();
      info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      info.image                           = types::get( ctx.depth_image_ );
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

    ctx.depth_image_view_ = types::create_unique_image_view( types::get( ctx.device_ ), depth_image_view_create_info );

    transition_layout( ctx,
                       types::decay( ctx.depth_image_ ),
                       VK_IMAGE_LAYOUT_UNDEFINED,
                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                       1 );
  }

  void init_framebuffers( context & ctx ) noexcept
  {
    ctx.framebuffers_.reserve( ctx.swapchain_images_count_ );

    auto const add_framebuffer = [&ctx]( auto const & image_view )
    {
      auto const attachments = std::array{ types::get( image_view ), types::get( ctx.depth_image_view_ ) };

      auto const framebuffer_create_info = [&ctx, &attachments]
      {
        auto info            = VkFramebufferCreateInfo();
        info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass      = types::get( ctx.render_pass_ );
        info.attachmentCount = static_cast<uint32_t>( std::size( attachments ) );
        info.pAttachments    = std::data( attachments );
        info.width           = ctx.swapchain_extent_.width;
        info.height          = ctx.swapchain_extent_.height;
        info.layers          = 1;
        return info;
      }();

      ctx.framebuffers_.push_back(
        types::create_unique_framebuffer( types::get( ctx.device_ ), framebuffer_create_info ) );
    };

    std::for_each( std::begin( ctx.swapchain_image_views_ ), std::end( ctx.swapchain_image_views_ ), add_framebuffer );
  }

  void init_main_render_pass( context & ctx ) noexcept
  {
    auto const color_attachment = [&ctx]
    {
      auto attachment           = VkAttachmentDescription();
      attachment.format         = ctx.surface_format_.format;
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

    auto const attachments = std::array{ color_attachment, depth_attachment };

    auto const render_pass_create_info = [&attachments, &subpass_description, &subpass_dependency]
    {
      auto info            = VkRenderPassCreateInfo();
      info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      info.attachmentCount = static_cast<uint32_t>( std::size( attachments ) );
      info.pAttachments    = std::data( attachments );
      info.subpassCount    = 1;
      info.pSubpasses      = &subpass_description;
      info.dependencyCount = 1;
      info.pDependencies   = &subpass_dependency;
      return info;
    }();

    ctx.render_pass_ = types::create_unique_render_pass( types::get( ctx.device_ ), render_pass_create_info );
  }

  void init_doesnt_belong_here( context & ctx ) noexcept
  {
    std::tie( ctx.texture_, ctx.width_, ctx.height_ ) = detail::load_texture( "../../assets/viking_room.png" );

    auto const image_create_info = [&ctx]
    {
      auto usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

      auto info          = VkImageCreateInfo();
      info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      info.imageType     = VK_IMAGE_TYPE_2D;
      info.extent.width  = ctx.width_;
      info.extent.height = ctx.height_;
      info.extent.depth  = 1;
      info.mipLevels     = detail::calculate_mimap_levels( ctx.width_, ctx.height_ );
      info.arrayLayers   = 1;
      info.format        = VK_FORMAT_R8G8B8A8_SRGB;
      info.tiling        = VK_IMAGE_TILING_OPTIMAL;
      info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      info.usage         = static_cast<VkFlags>( usage );
      info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
      info.samples       = VK_SAMPLE_COUNT_1_BIT;
      info.flags         = 0;
      return info;
    }();

    ctx.image_ = types::create_unique_image( types::get( ctx.device_ ), image_create_info );

    auto image_requirements = VkMemoryRequirements();
    vkGetImageMemoryRequirements( types::get( ctx.device_ ), types::get( ctx.image_ ), &image_requirements );

    auto const memory_type_index = detail::find_memory_type(
      types::get( ctx.physical_device_ ), image_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    MVK_VERIFY( memory_type_index.has_value() );

    auto image_memory_allocate_info = [image_requirements, memory_type_index]
    {
      auto info            = VkMemoryAllocateInfo();
      info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      info.allocationSize  = image_requirements.size;
      info.memoryTypeIndex = memory_type_index.value();
      return info;
    }();

    ctx.image_memory_ = types::create_unique_device_memory( types::get( ctx.device_ ), image_memory_allocate_info );
    vkBindImageMemory( types::get( ctx.device_ ), types::get( ctx.image_ ), types::get( ctx.image_memory_ ), 0 );

    transition_layout( ctx,
                       types::decay( ctx.image_ ),
                       VK_IMAGE_LAYOUT_UNDEFINED,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       image_create_info.mipLevels );

    auto staged_texture = staging_allocate( ctx, utility::as_bytes( ctx.texture_ ) );
    stage_image( ctx, staged_texture, ctx.width_, ctx.height_, types::decay( ctx.image_ ) );

    generate_mipmaps( ctx, types::decay( ctx.image_ ), ctx.width_, ctx.height_, image_create_info.mipLevels );

    auto const image_view_create_info = [&ctx, &image_create_info]
    {
      auto info                            = VkImageViewCreateInfo();
      info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      info.image                           = types::get( ctx.image_ );
      info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
      info.format                          = VK_FORMAT_R8G8B8A8_SRGB;
      info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      info.subresourceRange.baseMipLevel   = 0;
      info.subresourceRange.levelCount     = image_create_info.mipLevels;
      info.subresourceRange.baseArrayLayer = 0;
      info.subresourceRange.layerCount     = 1;
      return info;
    }();

    ctx.image_view_ = types::create_unique_image_view( types::decay( ctx.device_ ), image_view_create_info );

    auto const sampler_create_info = []
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
      info.maxLod                  = std::numeric_limits<float>::max();
      return info;
    }();

    ctx.sampler_ = types::create_unique_sampler( types::get( ctx.device_ ), sampler_create_info );

    std::tie( ctx.vertices_, ctx.indices_ ) = detail::read_object( "../../assets/viking_room.obj" );

    ctx.image_descriptor_set_ =
      std::move( allocate_descriptor_sets<1>( ctx, types::decay( ctx.texture_descriptor_set_layout_ ) )[0] );

    auto const image_descriptor_image_info = [&ctx]
    {
      auto info        = VkDescriptorImageInfo();
      info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      info.imageView   = types::get( ctx.image_view_ );
      info.sampler     = types::get( ctx.sampler_ );
      return info;
    }();

    auto const image_write = [&ctx, &image_descriptor_image_info]
    {
      auto write             = VkWriteDescriptorSet();
      write.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet           = types::get( ctx.image_descriptor_set_ );
      write.dstBinding       = 0;
      write.dstArrayElement  = 0;
      write.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      write.descriptorCount  = 1;
      write.pBufferInfo      = nullptr;
      write.pImageInfo       = &image_descriptor_image_info;
      write.pTexelBufferView = nullptr;
      return write;
    }();

    auto const descriptor_writes = std::array{ image_write };

    vkUpdateDescriptorSets( types::get( ctx.device_ ),
                            static_cast<uint32_t>( std::size( descriptor_writes ) ),
                            std::data( descriptor_writes ),
                            0,
                            nullptr );
  }

  void allocate_command_buffers( context & ctx ) noexcept
  {
    auto info               = VkCommandBufferAllocateInfo();
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool        = types::get( ctx.command_pool_ );
    info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = ctx.swapchain_images_count_;

    ctx.command_buffers_ = types::allocate_unique_command_buffers( types::decay( ctx.device_ ), info );
  }

  void init_shaders( context & ctx ) noexcept
  {
    auto const vertex_code = detail::read_file( "../../shaders/vert.spv" );

    auto const vertex_shader_create_info = [&vertex_code]
    {
      auto info     = VkShaderModuleCreateInfo();
      info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      info.codeSize = static_cast<uint32_t>( std::size( vertex_code ) );
      info.pCode    = reinterpret_cast<uint32_t const *>( std::data( vertex_code ) );
      return info;
    }();

    ctx.vertex_shader_ = types::create_unique_shader_module( types::get( ctx.device_ ), vertex_shader_create_info );

    auto const fragment_code = detail::read_file( "../../shaders/frag.spv" );

    auto const fragment_shader_create_info = [&fragment_code]
    {
      auto info     = VkShaderModuleCreateInfo();
      info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      info.codeSize = static_cast<uint32_t>( std::size( fragment_code ) );
      info.pCode    = reinterpret_cast<uint32_t const *>( std::data( fragment_code ) );
      return info;
    }();

    ctx.fragment_shader_ = types::create_unique_shader_module( types::get( ctx.device_ ), fragment_shader_create_info );
  }

  void init_pipeline( context & ctx ) noexcept
  {
    auto const vertex_input_binding_description = []
    {
      auto description      = VkVertexInputBindingDescription();
      description.binding   = 0;
      description.stride    = sizeof( vertex );
      description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
      return description;
    }();

    auto const position_attribute = []
    {
      auto attribute     = VkVertexInputAttributeDescription();
      attribute.binding  = 0;
      attribute.location = 0;
      attribute.format   = VK_FORMAT_R32G32B32_SFLOAT;
      attribute.offset   = offsetof( vertex, pos );
      return attribute;
    }();

    auto const color_attribute = []
    {
      auto attribute     = VkVertexInputAttributeDescription();
      attribute.binding  = 0;
      attribute.location = 1;
      attribute.format   = VK_FORMAT_R32G32B32_SFLOAT;
      attribute.offset   = offsetof( vertex, color );
      return attribute;
    }();

    auto const texture_coordinate_attribute = []
    {
      auto attribute     = VkVertexInputAttributeDescription();
      attribute.binding  = 0;
      attribute.location = 2;
      attribute.format   = VK_FORMAT_R32G32_SFLOAT;
      attribute.offset   = offsetof( vertex, texture_coord );
      return attribute;
    }();

    auto const vertex_attributes = std::array{ position_attribute, color_attribute, texture_coordinate_attribute };

    auto const vertex_input_create_info = [&vertex_input_binding_description, &vertex_attributes]
    {
      auto info                            = VkPipelineVertexInputStateCreateInfo();
      info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
      info.vertexBindingDescriptionCount   = 1;
      info.pVertexBindingDescriptions      = &vertex_input_binding_description;
      info.vertexAttributeDescriptionCount = static_cast<uint32_t>( std::size( vertex_attributes ) );
      info.pVertexAttributeDescriptions    = std::data( vertex_attributes );
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

    auto const viewport = [&ctx]
    {
      auto tmp     = VkViewport();
      tmp.x        = 0.0F;
      tmp.y        = 0.0F;
      tmp.width    = static_cast<float>( ctx.swapchain_extent_.width );
      tmp.height   = static_cast<float>( ctx.swapchain_extent_.height );
      tmp.minDepth = 0.0F;
      tmp.maxDepth = 1.0F;
      return tmp;
    }();

    auto const scissor = [&ctx]
    {
      auto tmp     = VkRect2D();
      tmp.offset.x = 0;
      tmp.offset.y = 0;
      tmp.extent   = ctx.swapchain_extent_;
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
      auto attachment = VkPipelineColorBlendAttachmentState();
      attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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

    auto const vertex_shader_stage = [&ctx]
    {
      auto info   = VkPipelineShaderStageCreateInfo();
      info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      info.stage  = VK_SHADER_STAGE_VERTEX_BIT;
      info.module = types::get( ctx.vertex_shader_ );
      info.pName  = "main";
      return info;
    }();

    auto const fragment_shader_stage = [&ctx]
    {
      auto info   = VkPipelineShaderStageCreateInfo();
      info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      info.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
      info.module = types::get( ctx.fragment_shader_ );
      info.pName  = "main";
      return info;
    }();

    auto const shader_stages = std::array{ vertex_shader_stage, fragment_shader_stage };

    auto const pipeline_create_info = [&]
    {
      auto info                = VkGraphicsPipelineCreateInfo();
      info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
      info.stageCount          = static_cast<uint32_t>( std::size( shader_stages ) );
      info.pStages             = std::data( shader_stages );
      info.pVertexInputState   = &vertex_input_create_info;
      info.pInputAssemblyState = &input_assembly_create_info;
      info.pViewportState      = &viewport_state_create_info;
      info.pRasterizationState = &rasterizer_create_info;
      info.pMultisampleState   = &multisampling_create_info;
      info.pDepthStencilState  = &depth_stencil_state_create_info;
      info.pColorBlendState    = &color_blend_create_info;
      info.pDynamicState       = nullptr;
      info.layout              = types::get( ctx.pipeline_layout_ );
      info.renderPass          = types::get( ctx.render_pass_ );
      info.subpass             = 0;
      info.basePipelineHandle  = nullptr;
      info.basePipelineIndex   = -1;
      return info;
    }();

    ctx.pipeline_ = types::create_unique_pipeline( types::get( ctx.device_ ), pipeline_create_info );
  }

  void init_sync( context & ctx ) noexcept
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

    for ( auto i = size_t( 0 ); i < context::max_frames_in_flight; ++i )
    {
      ctx.image_available_semaphores_[i] =
        types::create_unique_semaphore( types::get( ctx.device_ ), semaphore_create_info );
      ctx.render_finished_semaphores_[i] =
        types::create_unique_semaphore( types::get( ctx.device_ ), semaphore_create_info );
      ctx.frame_in_flight_fences_[i] = types::create_unique_fence( types::get( ctx.device_ ), fence_create_info );
    }

    ctx.image_in_flight_fences_.resize( ctx.swapchain_images_count_, nullptr );
  }

  void recreate_after_swapchain_change( context & ctx ) noexcept
  {
    ctx.swapchain_image_views_.clear();

    init_swapchain( ctx );
    init_depth_image( ctx );
    init_main_render_pass( ctx );

    ctx.framebuffers_.clear();

    init_framebuffers( ctx );
    allocate_command_buffers( ctx );

    init_pipeline( ctx );
    ctx.image_in_flight_fences_.clear();
    init_sync( ctx );
  }

  void test_run( context & ctx ) noexcept
  {
    while ( !ctx.window_.should_close() )
    {
      glfwPollEvents();

      begin_draw( ctx );

      auto const pvm = create_test_pvm( ctx );

      basic_draw(
        ctx, utility::as_bytes( ctx.vertices_ ), utility::as_bytes( ctx.indices_ ), utility::as_bytes( pvm ) );

      end_draw( ctx );
    }

    vkDeviceWaitIdle( types::get( ctx.device_ ) );
  }

  void begin_draw( context & ctx ) noexcept
  {
    auto const & image_available_semaphore = ctx.image_available_semaphores_[ctx.current_frame_index_];

    auto const current_image_index = detail::next_swapchain_image(
      types::get( ctx.device_ ), types::get( ctx.swapchain_ ), types::get( image_available_semaphore ), nullptr );

    if ( !current_image_index.has_value() )
    {
      recreate_after_swapchain_change( ctx );
      begin_draw( ctx );
      return;
    }

    ctx.current_image_index_ = current_image_index.value();

    auto clear_color_value  = VkClearValue();
    clear_color_value.color = { { 0.0F, 0.0F, 0.0F, 1.0F } };

    auto clear_depth_value         = VkClearValue();
    clear_depth_value.depthStencil = { 1.0F, 0 };

    auto const clear_values = std::array{ clear_color_value, clear_depth_value };

    auto const render_pass_begin_info = [&ctx, &clear_values]
    {
      auto info                = VkRenderPassBeginInfo();
      info.sType               = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      info.renderPass          = types::get( ctx.render_pass_ );
      info.framebuffer         = types::get( ctx.framebuffers_[ctx.current_image_index_] );
      info.renderArea.offset.x = 0;
      info.renderArea.offset.y = 0;
      info.renderArea.extent   = ctx.swapchain_extent_;
      info.clearValueCount     = static_cast<uint32_t>( std::size( clear_values ) );
      info.pClearValues        = std::data( clear_values );
      return info;
    }();

    auto const command_buffer_begin_info = []
    {
      auto info             = VkCommandBufferBeginInfo();
      info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      info.flags            = 0;
      info.pInheritanceInfo = nullptr;
      return info;
    }();

    // Recreate command buffer
    allocate_command_buffers( ctx );

    ctx.current_command_buffer_ = types::decay( ctx.command_buffers_[ctx.current_image_index_] );

    vkBeginCommandBuffer( types::get( ctx.current_command_buffer_ ), &command_buffer_begin_info );
    vkCmdBeginRenderPass(
      types::get( ctx.current_command_buffer_ ), &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE );
  }

  void basic_draw( context &                       ctx,
                   utility::slice<std::byte const> vertices,
                   utility::slice<std::byte const> indices,
                   utility::slice<std::byte const> pvm ) noexcept
  {
    auto vertex_stage                   = staging_allocate( ctx, vertices );
    auto [vertex_buffer, vertex_offset] = vertex_allocate( ctx, vertex_stage );

    auto index_stage                  = staging_allocate( ctx, indices );
    auto [index_buffer, index_offset] = index_allocate( ctx, index_stage );

    auto [uniform_set, uniform_offset] = uniform_allocate( ctx, utility::as_bytes( pvm ) );

    auto descriptor_sets = std::array{ types::get( uniform_set ), types::get( ctx.image_descriptor_set_ ) };

    vkCmdBindPipeline(
      types::get( ctx.current_command_buffer_ ), VK_PIPELINE_BIND_POINT_GRAPHICS, types::get( ctx.pipeline_ ) );
    vkCmdBindVertexBuffers(
      types::get( ctx.current_command_buffer_ ), 0, 1, &types::get( vertex_buffer ), &vertex_offset );
    vkCmdBindIndexBuffer(
      types::get( ctx.current_command_buffer_ ), types::get( index_buffer ), index_offset, VK_INDEX_TYPE_UINT32 );
    vkCmdBindDescriptorSets( types::get( ctx.current_command_buffer_ ),
                             VK_PIPELINE_BIND_POINT_GRAPHICS,
                             types::get( ctx.pipeline_layout_ ),
                             0,
                             static_cast<uint32_t>( std::size( descriptor_sets ) ),
                             std::data( descriptor_sets ),
                             1,
                             &uniform_offset );
    vkCmdDrawIndexed(
      types::get( ctx.current_command_buffer_ ), static_cast<uint32_t>( std::size( ctx.indices_ ) ), 1, 0, 0, 0 );
  }

  void end_draw( context & ctx ) noexcept
  {
    vkCmdEndRenderPass( types::get( ctx.current_command_buffer_ ) );
    vkEndCommandBuffer( types::get( ctx.current_command_buffer_ ) );
    // Wait for the image in flight to end if it is
    auto const * image_in_flight_fence = ctx.image_in_flight_fences_[ctx.current_image_index_];

    if ( image_in_flight_fence != nullptr )
    {
      vkWaitForFences( types::get( ctx.device_ ),
                       1,
                       &types::get( *image_in_flight_fence ),
                       VK_TRUE,
                       std::numeric_limits<int64_t>::max() );
    }

    ctx.image_in_flight_fences_[ctx.current_image_index_] = &ctx.frame_in_flight_fences_[ctx.current_frame_index_];

    // get current semaphores
    auto const image_available_semaphore = types::decay( ctx.image_available_semaphores_[ctx.current_frame_index_] );
    auto const render_finished_semaphore = types::decay( ctx.render_finished_semaphores_[ctx.current_frame_index_] );

    auto const wait_semaphores   = std::array{ types::get( image_available_semaphore ) };
    auto const signal_semaphores = std::array{ types::get( render_finished_semaphore ) };
    auto const wait_stages       = std::array<VkPipelineStageFlags, 1>{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    auto const command_buffers   = std::array{ types::get( ctx.current_command_buffer_ ) };

    auto const submit_info = [&wait_semaphores, &signal_semaphores, &wait_stages, &command_buffers]
    {
      auto info                 = VkSubmitInfo();
      info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      info.waitSemaphoreCount   = static_cast<uint32_t>( std::size( wait_semaphores ) );
      info.pWaitSemaphores      = std::data( wait_semaphores );
      info.pWaitDstStageMask    = std::data( wait_stages );
      info.commandBufferCount   = static_cast<uint32_t>( std::size( command_buffers ) );
      info.pCommandBuffers      = std::data( command_buffers );
      info.signalSemaphoreCount = static_cast<uint32_t>( std::size( signal_semaphores ) );
      info.pSignalSemaphores    = std::data( signal_semaphores );
      return info;
    }();

    vkResetFences( types::get( ctx.device_ ), 1, &types::get( ctx.frame_in_flight_fences_[ctx.current_frame_index_] ) );
    vkQueueSubmit( types::get( ctx.graphics_queue_ ),
                   1,
                   &submit_info,
                   types::get( ctx.frame_in_flight_fences_[ctx.current_frame_index_] ) );

    auto const present_signal_semaphores = std::array{ types::get( render_finished_semaphore ) };
    auto const swapchains                = std::array{ types::get( ctx.swapchain_ ) };
    auto const image_indices             = std::array{ ctx.current_image_index_ };

    auto const present_info = [&present_signal_semaphores, &swapchains, &image_indices]
    {
      auto info               = VkPresentInfoKHR();
      info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
      info.waitSemaphoreCount = static_cast<uint32_t>( std::size( present_signal_semaphores ) );
      info.pWaitSemaphores    = std::data( present_signal_semaphores );
      info.swapchainCount     = static_cast<uint32_t>( std::size( swapchains ) );
      info.pSwapchains        = std::data( swapchains );
      info.pImageIndices      = std::data( image_indices );
      info.pResults           = nullptr;
      return info;
    }();

    auto const present_result = vkQueuePresentKHR( types::get( ctx.present_queue_ ), &present_info );
    vkQueueWaitIdle( types::get( ctx.present_queue_ ) );

    auto const resized = ctx.window_.framebuffer_resized();
    auto const change_swapchain =
      ( present_result == VK_ERROR_OUT_OF_DATE_KHR ) || ( present_result == VK_SUBOPTIMAL_KHR );

    if ( change_swapchain || resized )
    {
      ctx.window_.set_framebuffer_resized( false );
      recreate_after_swapchain_change( ctx );
      return;
    }

    MVK_VERIFY( VK_SUCCESS == present_result );

    ctx.current_frame_index_ = ( ctx.current_frame_index_ + 1 ) % context::max_frames_in_flight;
    next_buffer( ctx );
  }

  [[nodiscard]] float current_time( context const & ctx ) noexcept
  {
    auto const current_time = std::chrono::high_resolution_clock::now();
    auto const delta_time   = current_time - ctx.start_time;
    return std::chrono::duration<float, std::chrono::seconds::period>( delta_time ).count();
  }

  [[nodiscard]] pvm create_test_pvm( context const & ctx ) noexcept
  {
    auto const time = current_time( ctx );

    constexpr auto turn_rate = glm::radians( 90.0F );

    auto ubo = pvm();

    ubo.model = glm::rotate( glm::mat4( 1.0F ), time * turn_rate, glm::vec3( 0.0F, 0.0F, 1.0F ) );
    ubo.view =
      glm::lookAt( glm::vec3( 2.0F, 2.0F, 2.0F ), glm::vec3( 0.0F, 0.0F, 0.0F ), glm::vec3( 0.0F, 0.0F, 1.0F ) );

    auto const ratio =
      static_cast<float>( ctx.swapchain_extent_.width ) / static_cast<float>( ctx.swapchain_extent_.height );

    ubo.proj = glm::perspective( glm::radians( 45.0F ), ratio, 0.1F, 10.0F );
    ubo.proj[1][1] *= -1;

    return ubo;
  }

  [[nodiscard]] types::unique_command_buffer allocate_single_use_command_buffer( context const & ctx ) noexcept
  {
    auto info               = VkCommandBufferAllocateInfo();
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool        = types::get( ctx.command_pool_ );
    info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = 1;
    return std::move( types::allocate_unique_command_buffers( types::get( ctx.device_ ), info )[0] );
  }

  void transition_layout( context const & ctx,
                          types::image    image,
                          VkImageLayout   old_layout,
                          VkImageLayout   new_layout,
                          uint32_t        mipmap_levels ) noexcept
  {
    auto const [image_memory_barrier, source_stage, destination_stage] = [old_layout, new_layout, &image, mipmap_levels]
    {
      auto barrier                = VkImageMemoryBarrier();
      barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrier.oldLayout           = old_layout;
      barrier.newLayout           = new_layout;
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.image               = types::get( image );

      if ( new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
      {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      }
      else
      {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      }

      barrier.subresourceRange.baseMipLevel   = 0;
      barrier.subresourceRange.levelCount     = mipmap_levels;
      barrier.subresourceRange.baseArrayLayer = 0;
      barrier.subresourceRange.layerCount     = 1;

      auto source_stage      = VkPipelineStageFlags();
      auto destination_stage = VkPipelineStageFlags();

      if ( old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
      {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      }
      else if ( old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
      {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      }
      else if ( old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
                new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
      {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask =
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        source_stage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      }
      else
      {
        MVK_VERIFY_NOT_REACHED();
      }

      return std::make_tuple( barrier, source_stage, destination_stage );
    }();

    auto const begin_info = []
    {
      auto info             = VkCommandBufferBeginInfo();
      info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      info.pInheritanceInfo = nullptr;
      return info;
    }();

    auto const command_buffer = allocate_single_use_command_buffer( ctx );

    vkBeginCommandBuffer( types::get( command_buffer ), &begin_info );
    vkCmdPipelineBarrier( types::get( command_buffer ),
                          source_stage,
                          destination_stage,
                          0,
                          0,
                          nullptr,
                          0,
                          nullptr,
                          1,
                          &image_memory_barrier );
    vkEndCommandBuffer( types::get( command_buffer ) );

    auto submit_info               = VkSubmitInfo();
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &types::get( command_buffer );

    vkQueueSubmit( types::get( ctx.graphics_queue_ ), 1, &submit_info, nullptr );
    vkQueueWaitIdle( types::get( ctx.graphics_queue_ ) );
  }

  void generate_mipmaps(
    context const & ctx, types::image image, uint32_t width, uint32_t height, uint32_t mipmap_levels ) noexcept
  {
    if ( mipmap_levels == 1 || mipmap_levels == 0 )
    {
      return;
    }

    auto const begin_info = []
    {
      auto info             = VkCommandBufferBeginInfo();
      info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      info.pInheritanceInfo = nullptr;
      return info;
    }();

    auto command_buffer = allocate_single_use_command_buffer( ctx );
    vkBeginCommandBuffer( types::get( command_buffer ), &begin_info );

    auto barrier                            = VkImageMemoryBarrier();
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = types::get( image );
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.subresourceRange.levelCount     = 1;

    auto mipmap_width  = static_cast<int32_t>( width );
    auto mipmap_height = static_cast<int32_t>( height );

    auto const half = []( auto & num )
    {
      if ( num > 1 )
      {
        num /= 2;
        return num;
      }

      return 1;
    };

    for ( auto i = uint32_t( 0 ); i < ( mipmap_levels - 1 ); ++i )
    {
      barrier.subresourceRange.baseMipLevel = i;
      barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

      vkCmdPipelineBarrier( types::get( command_buffer ),
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0,
                            0,
                            nullptr,
                            0,
                            nullptr,
                            1,
                            &barrier );

      auto blit                          = VkImageBlit();
      blit.srcOffsets[0].x               = 0;
      blit.srcOffsets[0].y               = 0;
      blit.srcOffsets[0].z               = 0;
      blit.srcOffsets[1].x               = mipmap_width;
      blit.srcOffsets[1].y               = mipmap_height;
      blit.srcOffsets[1].z               = 1;
      blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.srcSubresource.mipLevel       = i;
      blit.srcSubresource.baseArrayLayer = 0;
      blit.srcSubresource.layerCount     = 1;
      blit.dstOffsets[0].x               = 0;
      blit.dstOffsets[0].y               = 0;
      blit.dstOffsets[0].z               = 0;
      blit.dstOffsets[1].x               = half( mipmap_width );
      blit.dstOffsets[1].y               = half( mipmap_height );
      blit.dstOffsets[1].z               = 1;
      blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.dstSubresource.mipLevel       = i + 1;
      blit.dstSubresource.baseArrayLayer = 0;
      blit.dstSubresource.layerCount     = 1;

      vkCmdBlitImage( types::get( command_buffer ),
                      types::get( image ),
                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      types::get( image ),
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      1,
                      &blit,
                      VK_FILTER_LINEAR );

      barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      vkCmdPipelineBarrier( types::get( command_buffer ),
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                            0,
                            0,
                            nullptr,
                            0,
                            nullptr,
                            1,
                            &barrier );
    }

    barrier.subresourceRange.baseMipLevel = mipmap_levels - 1;
    barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier( types::get( command_buffer ),
                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                          0,
                          0,
                          nullptr,
                          0,
                          nullptr,
                          1,
                          &barrier );

    vkEndCommandBuffer( types::get( command_buffer ) );

    auto submit_info               = VkSubmitInfo();
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &types::get( command_buffer );

    vkQueueSubmit( types::get( ctx.graphics_queue_ ), 1, &submit_info, nullptr );
    vkQueueWaitIdle( types::get( ctx.graphics_queue_ ) );
  }

  // buffers
  void create_vertex_buffers_and_memories( context & ctx, types::device_size size ) noexcept
  {
    auto const vertex_buffer_create_info = [size]
    {
      auto info        = VkBufferCreateInfo();
      info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      info.size        = size;
      info.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
      info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      return info;
    }();

    auto const create_buffer = [&ctx, &vertex_buffer_create_info]
    {
      return types::create_unique_buffer( types::get( ctx.device_ ), vertex_buffer_create_info );
    };

    std::generate( std::begin( ctx.vertex_buffers_ ), std::end( ctx.vertex_buffers_ ), create_buffer );

    vkGetBufferMemoryRequirements( types::get( ctx.device_ ),
                                   types::get( ctx.vertex_buffers_[ctx.current_buffer_index_] ),
                                   &ctx.vertex_memory_requirements_ );

    ctx.vertex_aligned_size_ =
      detail::aligned_size( ctx.vertex_memory_requirements_.size, ctx.vertex_memory_requirements_.alignment );

    auto const memory_type_index = detail::find_memory_type( types::get( ctx.physical_device_ ),
                                                             ctx.vertex_memory_requirements_.memoryTypeBits,
                                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    MVK_VERIFY( memory_type_index.has_value() );

    auto vertex_memory_allocate_info = [&ctx, memory_type_index]
    {
      auto info            = VkMemoryAllocateInfo();
      info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      info.allocationSize  = context::dynamic_buffer_count * ctx.vertex_aligned_size_;
      info.memoryTypeIndex = memory_type_index.value();
      return info;
    }();

    ctx.vertex_memory_ = types::create_unique_device_memory( types::get( ctx.device_ ), vertex_memory_allocate_info );

    for ( size_t i = 0; i < context::dynamic_buffer_count; ++i )
    {
      auto const & current_buffer = ctx.vertex_buffers_[i];
      vkBindBufferMemory( types::parent( current_buffer ),
                          types::get( current_buffer ),
                          types::get( ctx.vertex_memory_ ),
                          i * ctx.vertex_aligned_size_ );
    }
  }

  void create_index_buffers_and_memories( context & ctx, types::device_size size ) noexcept
  {
    auto const index_buffer_create_info = [size]
    {
      auto info        = VkBufferCreateInfo();
      info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      info.size        = size;
      info.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
      info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      return info;
    }();

    auto const create_buffer = [&ctx, &index_buffer_create_info]
    {
      return types::create_unique_buffer( types::get( ctx.device_ ), index_buffer_create_info );
    };

    std::generate( std::begin( ctx.index_buffers_ ), std::end( ctx.index_buffers_ ), create_buffer );

    vkGetBufferMemoryRequirements( types::get( ctx.device_ ),
                                   types::get( ctx.index_buffers_[ctx.current_buffer_index_] ),
                                   &ctx.index_memory_requirements_ );

    ctx.index_aligned_size_ =
      detail::aligned_size( ctx.index_memory_requirements_.size, ctx.index_memory_requirements_.alignment );

    auto const memory_type_index = detail::find_memory_type( types::get( ctx.physical_device_ ),
                                                             ctx.index_memory_requirements_.memoryTypeBits,
                                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    MVK_VERIFY( memory_type_index.has_value() );

    auto index_memory_allocate_info = [&ctx, memory_type_index]
    {
      auto info            = VkMemoryAllocateInfo();
      info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      info.allocationSize  = context::dynamic_buffer_count * ctx.index_aligned_size_;
      info.memoryTypeIndex = memory_type_index.value();
      return info;
    }();

    ctx.index_memory_ = types::create_unique_device_memory( types::get( ctx.device_ ), index_memory_allocate_info );

    for ( size_t i = 0; i < context::dynamic_buffer_count; ++i )
    {
      auto const & current_buffer = ctx.index_buffers_[i];
      vkBindBufferMemory( types::parent( current_buffer ),
                          types::get( current_buffer ),
                          types::get( ctx.index_memory_ ),
                          i * ctx.index_aligned_size_ );
    }
  }

  void create_staging_buffers_and_memories( context & ctx, types::device_size size ) noexcept
  {
    auto const staging_buffer_create_info = [size]
    {
      auto info        = VkBufferCreateInfo();
      info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      info.size        = size;
      info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
      info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      return info;
    }();

    auto const create_buffer = [&ctx, &staging_buffer_create_info]
    {
      return types::create_unique_buffer( types::get( ctx.device_ ), staging_buffer_create_info );
    };

    std::generate( std::begin( ctx.staging_buffers_ ), std::end( ctx.staging_buffers_ ), create_buffer );

    vkGetBufferMemoryRequirements( types::get( ctx.device_ ),
                                   types::get( ctx.staging_buffers_[ctx.current_buffer_index_] ),
                                   &ctx.staging_memory_requirements_ );

    ctx.staging_aligned_size_ =
      detail::aligned_size( ctx.staging_memory_requirements_.size, ctx.staging_memory_requirements_.alignment );

    auto const memory_type_index =
      detail::find_memory_type( types::get( ctx.physical_device_ ),
                                ctx.staging_memory_requirements_.memoryTypeBits,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    MVK_VERIFY( memory_type_index.value() );

    auto staging_memory_allocate_info = [&ctx, memory_type_index]
    {
      auto info            = VkMemoryAllocateInfo();
      info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      info.allocationSize  = context::dynamic_buffer_count * ctx.staging_aligned_size_;
      info.memoryTypeIndex = memory_type_index.value();
      return info;
    }();

    ctx.staging_memory_ = types::create_unique_device_memory( types::get( ctx.device_ ), staging_memory_allocate_info );
    ctx.staging_data_   = detail::map_memory( types::decay( ctx.device_ ), types::decay( ctx.staging_memory_ ) );

    for ( size_t i = 0; i < context::dynamic_buffer_count; ++i )
    {
      auto const & current_buffer = ctx.staging_buffers_[i];
      vkBindBufferMemory( types::parent( current_buffer ),
                          types::get( current_buffer ),
                          types::get( ctx.staging_memory_ ),
                          i * ctx.staging_aligned_size_ );
    }
  }

  void move_to_garbage_buffers( context &                                                           ctx,
                                utility::slice<types::unique_buffer, context::dynamic_buffer_count> buffers ) noexcept
  {
    auto & garbage_buffers = ctx.garbage_buffers_[ctx.current_garbage_index_];
    garbage_buffers.reserve( std::size( garbage_buffers ) + std::size( buffers ) );

    for ( auto & buffer : buffers )
    {
      ctx.garbage_buffers_[ctx.current_garbage_index_].push_back( std::move( buffer ) );
    }
  }

  void move_to_garbage_descriptor_sets(
    context & ctx, utility::slice<types::unique_descriptor_set, context::dynamic_buffer_count> sets ) noexcept
  {
    auto & garbage_descriptor_sets = ctx.garbage_descriptor_sets_[ctx.current_garbage_index_];
    garbage_descriptor_sets.reserve( std::size( garbage_descriptor_sets ) + std::size( sets ) );

    for ( auto & set : sets )
    {
      ctx.garbage_descriptor_sets_[ctx.current_garbage_index_].push_back( std::move( set ) );
    }
  }

  void move_to_garbage_memories( context & ctx, types::unique_device_memory memory ) noexcept
  {
    ctx.garbage_memories_[ctx.current_garbage_index_].push_back( std::move( memory ) );
  }

  [[nodiscard]] staging_allocation staging_allocate( context & ctx, utility::slice<std::byte const> src ) noexcept
  {
    ctx.staging_offsets_[ctx.current_buffer_index_] = detail::aligned_size(
      ctx.staging_offsets_[ctx.current_buffer_index_], ctx.staging_memory_requirements_.alignment );

    if ( auto required_size = ctx.staging_offsets_[ctx.current_buffer_index_] + std::size( src );
         required_size > ctx.staging_aligned_size_ )
    {
      // TODO(samuel): should require a move
      move_to_garbage_buffers( ctx, ctx.staging_buffers_ );
      move_to_garbage_memories( ctx, std::move( ctx.staging_memory_ ) );
      create_staging_buffers_and_memories( ctx, required_size * 2 );
      ctx.staging_offsets_[ctx.current_buffer_index_] = 0;
    }

    auto const memory_offset =
      ctx.current_buffer_index_ * ctx.staging_aligned_size_ + ctx.staging_offsets_[ctx.current_buffer_index_];
    auto data = ctx.staging_data_.subslice( memory_offset, std::size( src ) );
    std::copy( std::begin( src ), std::end( src ), std::begin( data ) );

    return { types::decay( ctx.staging_buffers_[ctx.current_buffer_index_] ),
             std::exchange( ctx.staging_offsets_[ctx.current_buffer_index_],
                            ctx.staging_offsets_[ctx.current_buffer_index_] + std::size( src ) ),
             std::size( src ) };
  }

  void stage_image(
    context & ctx, staging_allocation allocation, uint32_t width, uint32_t height, types::image image ) noexcept
  {
    auto const begin_info = []
    {
      auto info             = VkCommandBufferBeginInfo();
      info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      info.pInheritanceInfo = nullptr;
      return info;
    }();

    auto const copy_region = [width, height, allocation]
    {
      auto region                            = VkBufferImageCopy();
      region.bufferOffset                    = allocation.offset_;
      region.bufferRowLength                 = 0;
      region.bufferImageHeight               = 0;
      region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      region.imageSubresource.mipLevel       = 0;
      region.imageSubresource.baseArrayLayer = 0;
      region.imageSubresource.layerCount     = 1;
      region.imageOffset.x                   = 0;
      region.imageOffset.y                   = 0;
      region.imageOffset.z                   = 0;
      region.imageExtent.width               = width;
      region.imageExtent.height              = height;
      region.imageExtent.depth               = 1;
      return region;
    }();

    auto const command_buffer = allocate_single_use_command_buffer( ctx );

    vkBeginCommandBuffer( types::get( command_buffer ), &begin_info );

    vkCmdCopyBufferToImage( types::get( command_buffer ),
                            types::get( allocation.buffer_ ),
                            types::get( image ),
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            1,
                            &copy_region );

    vkEndCommandBuffer( types::get( command_buffer ) );

    auto submit_info               = VkSubmitInfo();
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &types::get( command_buffer );

    vkQueueSubmit( types::get( ctx.graphics_queue_ ), 1, &submit_info, nullptr );
    vkQueueWaitIdle( types::get( ctx.graphics_queue_ ) );
  }

  [[nodiscard]] vertex_allocation vertex_allocate( context & ctx, staging_allocation allocation ) noexcept
  {
    if ( auto required_size = ctx.vertex_offsets_[ctx.current_buffer_index_] + allocation.size_;
         required_size > ctx.vertex_aligned_size_ )
    {
      move_to_garbage_buffers( ctx, ctx.vertex_buffers_ );
      move_to_garbage_memories( ctx, std::move( ctx.vertex_memory_ ) );
      create_vertex_buffers_and_memories( ctx, required_size * 2 );
      ctx.vertex_offsets_[ctx.current_buffer_index_] = 0;
    }

    auto const begin_info = []
    {
      auto info             = VkCommandBufferBeginInfo();
      info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      info.pInheritanceInfo = nullptr;
      return info;
    }();

    auto const copy_region = [allocation, &ctx]
    {
      auto region      = VkBufferCopy();
      region.srcOffset = allocation.offset_;
      region.dstOffset = ctx.vertex_offsets_[ctx.current_buffer_index_];
      region.size      = allocation.size_;
      return region;
    }();

    auto const command_buffer = allocate_single_use_command_buffer( ctx );
    vkBeginCommandBuffer( types::get( command_buffer ), &begin_info );

    vkCmdCopyBuffer( types::get( command_buffer ),
                     types::get( allocation.buffer_ ),
                     types::get( ctx.vertex_buffers_[ctx.current_buffer_index_] ),
                     1,
                     &copy_region );

    vkEndCommandBuffer( types::get( command_buffer ) );

    auto submit_info               = VkSubmitInfo();
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &types::get( command_buffer );

    vkQueueSubmit( types::get( ctx.graphics_queue_ ), 1, &submit_info, nullptr );
    vkQueueWaitIdle( types::get( ctx.graphics_queue_ ) );

    return { types::decay( ctx.vertex_buffers_[ctx.current_buffer_index_] ),
             std::exchange( ctx.vertex_offsets_[ctx.current_buffer_index_],
                            ctx.vertex_offsets_[ctx.current_buffer_index_] + allocation.size_ ) };
  }

  [[nodiscard]] index_allocation index_allocate( context & ctx, staging_allocation allocation ) noexcept
  {
    if ( auto required_size = ctx.index_offsets_[ctx.current_buffer_index_] + allocation.size_;
         required_size > ctx.index_aligned_size_ )
    {
      move_to_garbage_buffers( ctx, ctx.index_buffers_ );
      move_to_garbage_memories( ctx, std::move( ctx.index_memory_ ) );
      create_index_buffers_and_memories( ctx, required_size * 2 );
      ctx.index_offsets_[ctx.current_buffer_index_] = 0;
    }

    auto const begin_info = []
    {
      auto info             = VkCommandBufferBeginInfo();
      info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      info.pInheritanceInfo = nullptr;
      return info;
    }();

    auto const copy_region = [allocation, &ctx]
    {
      auto region      = VkBufferCopy();
      region.srcOffset = allocation.offset_;
      region.dstOffset = ctx.index_offsets_[ctx.current_buffer_index_];
      region.size      = allocation.size_;
      return region;
    }();

    auto const command_buffer = allocate_single_use_command_buffer( ctx );
    vkBeginCommandBuffer( types::get( command_buffer ), &begin_info );

    vkCmdCopyBuffer( types::get( command_buffer ),
                     types::get( allocation.buffer_ ),
                     types::get( ctx.index_buffers_[ctx.current_buffer_index_] ),
                     1,
                     &copy_region );

    vkEndCommandBuffer( types::get( command_buffer ) );

    auto submit_info               = VkSubmitInfo();
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &types::get( command_buffer );

    vkQueueSubmit( types::get( ctx.graphics_queue_ ), 1, &submit_info, nullptr );
    vkQueueWaitIdle( types::get( ctx.graphics_queue_ ) );

    return { types::decay( ctx.index_buffers_[ctx.current_buffer_index_] ),
             std::exchange( ctx.index_offsets_[ctx.current_buffer_index_],
                            ctx.index_offsets_[ctx.current_buffer_index_] + allocation.size_ ) };
  }

  void create_uniform_buffers_memories_and_sets( context & ctx, types::device_size size ) noexcept
  {
    auto const uniform_buffer_create_info = [size]
    {
      auto info        = VkBufferCreateInfo();
      info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      info.size        = size;
      info.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
      info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      return info;
    }();

    auto const create_buffer = [&ctx, &uniform_buffer_create_info]
    {
      return types::create_unique_buffer( types::get( ctx.device_ ), uniform_buffer_create_info );
    };

    std::generate( std::begin( ctx.uniform_buffers_ ), std::end( ctx.uniform_buffers_ ), create_buffer );

    vkGetBufferMemoryRequirements( types::get( ctx.device_ ),
                                   types::get( ctx.uniform_buffers_[ctx.current_buffer_index_] ),
                                   &ctx.uniform_memory_requirements_ );

    ctx.uniform_aligned_size_ =
      detail::aligned_size( ctx.uniform_memory_requirements_.size, ctx.uniform_memory_requirements_.alignment );

    auto const memory_type_index =
      detail::find_memory_type( types::get( ctx.physical_device_ ),
                                ctx.uniform_memory_requirements_.memoryTypeBits,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    MVK_VERIFY( memory_type_index.value() );

    auto uniform_memory_allocate_info = [&ctx, memory_type_index]
    {
      auto info            = VkMemoryAllocateInfo();
      info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      info.allocationSize  = context::dynamic_buffer_count * ctx.uniform_aligned_size_;
      info.memoryTypeIndex = memory_type_index.value();
      return info;
    }();

    ctx.uniform_memory_ = types::create_unique_device_memory( types::get( ctx.device_ ), uniform_memory_allocate_info );
    ctx.uniform_data_   = detail::map_memory( types::decay( ctx.device_ ), types::decay( ctx.uniform_memory_ ) );
    ctx.uniform_descriptor_sets_ = allocate_descriptor_sets<context::dynamic_buffer_count>(
      ctx, types::decay( ctx.uniform_descriptor_set_layout_ ) );

    for ( size_t i = 0; i < context::dynamic_buffer_count; ++i )
    {
      vkBindBufferMemory( types::parent( ctx.uniform_buffers_[i] ),
                          types::get( ctx.uniform_buffers_[i] ),
                          types::get( ctx.uniform_memory_ ),
                          i * ctx.uniform_aligned_size_ );

      auto const uniform_descriptor_buffer_info = [&ctx, i]
      {
        auto info   = VkDescriptorBufferInfo();
        info.buffer = types::get( ctx.uniform_buffers_[i] );
        info.offset = 0;
        info.range  = sizeof( pvm );
        return info;
      }();

      auto const uniform_write = [&ctx, &uniform_descriptor_buffer_info, i]
      {
        auto write             = VkWriteDescriptorSet();
        write.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet           = types::get( ctx.uniform_descriptor_sets_[i] );
        write.dstBinding       = 0;
        write.dstArrayElement  = 0;
        write.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write.descriptorCount  = 1;
        write.pBufferInfo      = &uniform_descriptor_buffer_info;
        write.pImageInfo       = nullptr;
        write.pTexelBufferView = nullptr;
        return write;
      }();

      auto const descriptor_writes = std::array{ uniform_write };

      vkUpdateDescriptorSets( types::get( ctx.device_ ),
                              static_cast<uint32_t>( std::size( descriptor_writes ) ),
                              std::data( descriptor_writes ),
                              0,
                              nullptr );
    }
  }

  [[nodiscard]] uniform_allocation uniform_allocate( context & ctx, utility::slice<std::byte const> src ) noexcept
  {
    ctx.uniform_offsets_[ctx.current_buffer_index_] =
      detail::aligned_size( ctx.uniform_offsets_[ctx.current_buffer_index_],
                            static_cast<uint32_t>( ctx.uniform_memory_requirements_.alignment ) );

    if ( auto required_size = ctx.uniform_offsets_[ctx.current_buffer_index_] + std::size( src );
         required_size > ctx.uniform_aligned_size_ )
    {
      // TODO(samuel): should require a move
      move_to_garbage_buffers( ctx, ctx.uniform_buffers_ );
      move_to_garbage_descriptor_sets( ctx, ctx.uniform_descriptor_sets_ );

      move_to_garbage_memories( ctx, std::move( ctx.uniform_memory_ ) );
      create_uniform_buffers_memories_and_sets( ctx, required_size * 2 );
      ctx.uniform_offsets_[ctx.current_buffer_index_] = 0;
    }

    auto const memory_offset =
      ctx.current_buffer_index_ * ctx.uniform_aligned_size_ + ctx.uniform_offsets_[ctx.current_buffer_index_];
    auto data = ctx.uniform_data_.subslice( memory_offset, std::size( src ) );
    std::copy( std::begin( src ), std::end( src ), std::begin( data ) );

    return { types::decay( ctx.uniform_descriptor_sets_[ctx.current_buffer_index_] ),
             std::exchange( ctx.uniform_offsets_[ctx.current_buffer_index_],
                            ctx.uniform_offsets_[ctx.current_buffer_index_] + std::size( src ) ) };
  }

  void next_buffer( context & ctx ) noexcept
  {
    ctx.current_buffer_index_ = ( ctx.current_buffer_index_ + 1 ) % context::dynamic_buffer_count;

    ctx.staging_offsets_[ctx.current_buffer_index_] = 0;
    ctx.vertex_offsets_[ctx.current_buffer_index_]  = 0;
    ctx.index_offsets_[ctx.current_buffer_index_]   = 0;
  }

}  // namespace mvk
