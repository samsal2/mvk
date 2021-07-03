#include "engine/init.hpp"

#include "detail/helpers.hpp"
#include "detail/misc.hpp"
#include "detail/readers.hpp"
#include "engine/allocate.hpp"
#include "engine/context.hpp"
#include "engine/debug.hpp"
#include "engine/image.hpp"
#include "utility/verify.hpp"

namespace mvk::engine
{
  [[nodiscard]] context create_context( char const * name, VkExtent2D extent ) noexcept
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
    init_render_pass( ctx );
    init_framebuffers( ctx );
    init_samplers( ctx );
    init_doesnt_belong_here( ctx );
    init_command_buffers( ctx );
    init_shaders( ctx );
    init_pipeline( ctx );
    init_sync( ctx );
    create_vertex_buffers_and_memories( ctx, 1024 * 1024 );
    create_index_buffers_and_memories( ctx, 1024 * 1024 );
    create_uniform_buffers_memories_and_sets( ctx, 1024 * 1024 );
    return ctx;
  }

  void destroy_context( context & ctx ) noexcept
  {
    destroy_garbage_sets( ctx );
    destroy_garbage_memories( ctx );
    destroy_garbage_buffers( ctx );
    destroy_uniform_buffers_memories_and_sets( ctx );
    destroy_index_buffers_and_memories( ctx );
    destroy_vertex_buffers_and_memories( ctx );
    destroy_sync( ctx );
    destroy_pipelines( ctx );
    destroy_shaders( ctx );
    destroy_command_buffers( ctx );
    destroy_doesnt_beling_here( ctx );
    destroy_samplers( ctx );
    destroy_framebuffers( ctx );
    destroy_render_pass( ctx );
    destroy_depth_image( ctx );
    destroy_swapchain( ctx );
    destroy_pools( ctx );
    destroy_layouts( ctx );
    destroy_staging_buffers_and_memories( ctx );
    destroy_device( ctx );
    destroy_surface( ctx );
    destroy_debug_messenger( ctx );
    destroy_instance( ctx );
    destroy_window( ctx );
  }

  void init_window( context & ctx, VkExtent2D extent ) noexcept
  {
    glfwInit();
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

    auto const callback = []( GLFWwindow * const         window_ptr,
                              [[maybe_unused]] int const callback_width,
                              [[maybe_unused]] int const callback_height )
    {
      auto const user_ptr    = glfwGetWindowUserPointer( window_ptr );
      auto const c           = reinterpret_cast< context * >( user_ptr );
      c->framebuffer_resized = true;
    };

    ctx.window = glfwCreateWindow(
      static_cast< int >( extent.width ), static_cast< int >( extent.height ), "stan loona", nullptr, nullptr );

    glfwSetWindowUserPointer( ctx.window, &ctx );
    glfwSetFramebufferSizeCallback( ctx.window, callback );
  }

  void destroy_window( context & ctx ) noexcept
  {
    glfwDestroyWindow( ctx.window );
    glfwTerminate();
  }

  void init_instance( context & ctx, char const * name ) noexcept
  {
    if constexpr ( context::use_validation )
    {
      auto validation_layer_properties_count = uint32_t( 0 );
      vkEnumerateInstanceLayerProperties( &validation_layer_properties_count, nullptr );

      auto validation_layer_properties = std::vector< VkLayerProperties >( validation_layer_properties_count );
      vkEnumerateInstanceLayerProperties( &validation_layer_properties_count,
                                          std::data( validation_layer_properties ) );

      auto const is_layer_present = []( auto const layer, utility::slice< char const * const > const layers ) noexcept
      {
        auto const matches = [ &layer ]( auto const & current_layer )
        {
          auto const layer_name_data = std::data( layer.layerName );
          return std::strcmp( current_layer, layer_name_data ) == 0;
        };
        return std::any_of( std::begin( layers ), std::end( layers ), matches );
      };

      auto const exists = [ is_layer_present ]( auto const & available_layer )
      {
        return is_layer_present( available_layer, context::validation_layers );
      };

      [[maybe_unused]] auto result =
        std::any_of( std::begin( validation_layer_properties ), std::end( validation_layer_properties ), exists );
      MVK_VERIFY( result );
    }

    auto application_info               = VkApplicationInfo();
    application_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName   = name;
    application_info.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    application_info.pEngineName        = "No Engine";
    application_info.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );

    auto       count = uint32_t( 0 );
    auto const data  = glfwGetRequiredInstanceExtensions( &count );

    auto required_extensions = std::vector< char const * >( data, std::next( data, count ) );

    if constexpr ( context::use_validation )
    {
      required_extensions.insert( std::begin( required_extensions ),
                                  std::begin( context::validation_instance_extensions ),
                                  std::end( context::validation_instance_extensions ) );
    }

    auto instance_create_info  = VkInstanceCreateInfo();
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    if constexpr ( context::use_validation )
    {
      instance_create_info.pNext = &debug_create_info;
    }
    else
    {
      instance_create_info.pNext = nullptr;
    }

    instance_create_info.pApplicationInfo = &application_info;

    if constexpr ( context::use_validation )
    {
      instance_create_info.enabledLayerCount   = static_cast< uint32_t >( std::size( context::validation_layers ) );
      instance_create_info.ppEnabledLayerNames = std::data( context::validation_layers );
    }
    else
    {
      instance_create_info.enabledLayerCount       = 0;
      instance_create_info.ppEnabledExtensionNames = nullptr;
    }

    instance_create_info.enabledExtensionCount   = static_cast< uint32_t >( std::size( required_extensions ) );
    instance_create_info.ppEnabledExtensionNames = std::data( required_extensions );

    [[maybe_unused]] auto result = vkCreateInstance( &instance_create_info, nullptr, &ctx.instance );
    MVK_VERIFY( result == VK_SUCCESS );
  }

  void destroy_instance( context & ctx ) noexcept
  {
    vkDestroyInstance( ctx.instance, nullptr );
  }

  void init_debug_messenger( context & ctx ) noexcept
  {
    if constexpr ( context::use_validation )
    {
      auto const create_debug_utils_messenger = reinterpret_cast< PFN_vkCreateDebugUtilsMessengerEXT >(
        vkGetInstanceProcAddr( ctx.instance, "vkCreateDebugUtilsMessengerEXT" ) );

      MVK_VERIFY( create_debug_utils_messenger );

      create_debug_utils_messenger( ctx.instance, &debug_create_info, nullptr, &ctx.debug_messenger );
    }
  }

  void destroy_debug_messenger( context & ctx ) noexcept
  {
    if constexpr ( context::use_validation )
    {
      auto const destroy_debug_utils_messenger = reinterpret_cast< PFN_vkDestroyDebugUtilsMessengerEXT >(
        vkGetInstanceProcAddr( ctx.instance, "vkDestroyDebugUtilsMessengerEXT" ) );

      MVK_VERIFY( destroy_debug_utils_messenger );

      destroy_debug_utils_messenger( ctx.instance, ctx.debug_messenger, nullptr );
    }
  }

  void init_surface( context & ctx ) noexcept
  {
    glfwCreateWindowSurface( ctx.instance, ctx.window, nullptr, &ctx.surface );
  }

  void destroy_surface( context & ctx ) noexcept
  {
    vkDestroySurfaceKHR( ctx.instance, ctx.surface, nullptr );
  }

  void select_physical_device( context & ctx ) noexcept
  {
    auto physical_device_count = uint32_t( 0 );
    vkEnumeratePhysicalDevices( ctx.instance, &physical_device_count, nullptr );

    auto physical_devices = std::vector< VkPhysicalDevice >( physical_device_count );
    vkEnumeratePhysicalDevices( ctx.instance, &physical_device_count, std::data( physical_devices ) );

    auto const supported = [ &ctx ]( auto const physical_device )
    {
      auto features = VkPhysicalDeviceFeatures();
      vkGetPhysicalDeviceFeatures( physical_device, &features );

      auto const extensions_supported = detail::check_extension_support( physical_device, context::device_extensions );
      auto const format_and_present_mode_available =
        detail::check_format_and_present_mode_availability( physical_device, ctx.surface );
      auto const family_indices_found = detail::query_family_indices( physical_device, ctx.surface ).has_value();

      return extensions_supported && format_and_present_mode_available && family_indices_found &&
             features.samplerAnisotropy;
    };

    auto const it = std::find_if( std::begin( physical_devices ), std::end( physical_devices ), supported );

    MVK_VERIFY( it != std::end( physical_devices ) );

    ctx.physical_device = *it;
  }

  void select_surface_format( context & ctx ) noexcept
  {
    auto formats_count = uint32_t( 0 );
    vkGetPhysicalDeviceSurfaceFormatsKHR( ctx.physical_device, ctx.surface, &formats_count, nullptr );

    auto formats = std::vector< VkSurfaceFormatKHR >( formats_count );
    vkGetPhysicalDeviceSurfaceFormatsKHR( ctx.physical_device, ctx.surface, &formats_count, std::data( formats ) );

    auto const meets_requirements = []( auto const & format )
    {
      return ( format.format == VK_FORMAT_B8G8R8A8_SRGB ) && ( format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR );
    };

    auto const it = std::find_if( std::begin( formats ), std::end( formats ), meets_requirements );

    ctx.surface_format = ( it != std::end( formats ) ) ? *it : formats[ 0 ];
  }

  void init_device( context & ctx ) noexcept
  {
    auto const queue_indices_result = detail::query_family_indices( ctx.physical_device, ctx.surface );

    MVK_VERIFY( queue_indices_result.has_value() );

    auto const queue_indices = queue_indices_result.value();
    ctx.graphics_queue_index = queue_indices.first;
    ctx.present_queue_index  = queue_indices.second;

    auto features = VkPhysicalDeviceFeatures();
    vkGetPhysicalDeviceFeatures( ctx.physical_device, &features );

    auto const queue_priority = 1.0F;

    auto graphics_queue_create_info             = VkDeviceQueueCreateInfo();
    graphics_queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphics_queue_create_info.queueFamilyIndex = ctx.graphics_queue_index;
    graphics_queue_create_info.queueCount       = 1;
    graphics_queue_create_info.pQueuePriorities = &queue_priority;

    auto present_queue_create_info             = VkDeviceQueueCreateInfo();
    present_queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    present_queue_create_info.queueFamilyIndex = ctx.present_queue_index;
    present_queue_create_info.queueCount       = 1;
    present_queue_create_info.pQueuePriorities = &queue_priority;

    auto const queue_create_info       = std::array{ graphics_queue_create_info, present_queue_create_info };
    auto const queue_create_info_count = static_cast< uint32_t >( queue_indices.first != queue_indices.second ? 2 : 1 );

    auto device_create_info                    = VkDeviceCreateInfo();
    device_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount    = queue_create_info_count;
    device_create_info.pQueueCreateInfos       = std::data( queue_create_info );
    device_create_info.pEnabledFeatures        = &features;
    device_create_info.enabledExtensionCount   = static_cast< uint32_t >( std::size( context::device_extensions ) );
    device_create_info.ppEnabledExtensionNames = std::data( context::device_extensions );

    if constexpr ( context::use_validation )
    {
      device_create_info.enabledLayerCount   = static_cast< uint32_t >( std::size( context::validation_layers ) );
      device_create_info.ppEnabledLayerNames = std::data( context::validation_layers );
    }
    else
    {
      device_create_info.enabledLayerCount   = 0;
      device_create_info.ppEnabledLayerNames = nullptr;
    }

    [[maybe_unused]] auto result = vkCreateDevice( ctx.physical_device, &device_create_info, nullptr, &ctx.device );
    MVK_VERIFY( result == VK_SUCCESS );

    vkGetDeviceQueue( ctx.device, ctx.graphics_queue_index, 0, &ctx.graphics_queue );
    vkGetDeviceQueue( ctx.device, ctx.present_queue_index, 0, &ctx.present_queue );
  }

  void destroy_device( context & ctx ) noexcept
  {
    vkDestroyDevice( ctx.device, nullptr );
  }

  void init_layouts( context & ctx ) noexcept
  {
    auto uniform_descriptor_set_layout_binding               = VkDescriptorSetLayoutBinding();
    uniform_descriptor_set_layout_binding.binding            = 0;
    uniform_descriptor_set_layout_binding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    uniform_descriptor_set_layout_binding.descriptorCount    = 1;
    uniform_descriptor_set_layout_binding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    uniform_descriptor_set_layout_binding.pImmutableSamplers = nullptr;

    auto uniform_descriptor_set_layout_create_info         = VkDescriptorSetLayoutCreateInfo();
    uniform_descriptor_set_layout_create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    uniform_descriptor_set_layout_create_info.bindingCount = 1;
    uniform_descriptor_set_layout_create_info.pBindings    = &uniform_descriptor_set_layout_binding;

    auto result = vkCreateDescriptorSetLayout(
      ctx.device, &uniform_descriptor_set_layout_create_info, nullptr, &ctx.uniform_descriptor_set_layout );

    MVK_VERIFY( result == VK_SUCCESS );

    auto sampler_descriptor_set_layout_binding               = VkDescriptorSetLayoutBinding();
    sampler_descriptor_set_layout_binding.binding            = 0;
    sampler_descriptor_set_layout_binding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_descriptor_set_layout_binding.descriptorCount    = 1;
    sampler_descriptor_set_layout_binding.pImmutableSamplers = nullptr;
    sampler_descriptor_set_layout_binding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

    auto sampler_descriptor_set_layout_create_info         = VkDescriptorSetLayoutCreateInfo();
    sampler_descriptor_set_layout_create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    sampler_descriptor_set_layout_create_info.bindingCount = 1;
    sampler_descriptor_set_layout_create_info.pBindings    = &sampler_descriptor_set_layout_binding;

    result = vkCreateDescriptorSetLayout(
      ctx.device, &sampler_descriptor_set_layout_create_info, nullptr, &ctx.texture_descriptor_set_layout );

    MVK_VERIFY( result == VK_SUCCESS );

    auto descriptor_set_layouts = std::array{ ctx.uniform_descriptor_set_layout, ctx.texture_descriptor_set_layout };

    auto pipeline_layout_create_info                   = VkPipelineLayoutCreateInfo();
    pipeline_layout_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount         = static_cast< uint32_t >( std::size( descriptor_set_layouts ) );
    pipeline_layout_create_info.pSetLayouts            = std::data( descriptor_set_layouts );
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges    = nullptr;

    result = vkCreatePipelineLayout( ctx.device, &pipeline_layout_create_info, nullptr, &ctx.pipeline_layout );

    MVK_VERIFY( result == VK_SUCCESS );
  }

  void destroy_layouts( context & ctx ) noexcept
  {
    vkDestroyPipelineLayout( ctx.device, ctx.pipeline_layout, nullptr );
    vkDestroyDescriptorSetLayout( ctx.device, ctx.texture_descriptor_set_layout, nullptr );
    vkDestroyDescriptorSetLayout( ctx.device, ctx.uniform_descriptor_set_layout, nullptr );
  }

  void init_pools( context & ctx ) noexcept
  {
    auto command_pool_create_info             = VkCommandPoolCreateInfo();
    command_pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.queueFamilyIndex = ctx.graphics_queue_index;
    command_pool_create_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    [[maybe_unused]] auto command_pool_result =
      vkCreateCommandPool( ctx.device, &command_pool_create_info, nullptr, &ctx.command_pool );
    MVK_VERIFY( command_pool_result == VK_SUCCESS );

    auto uniform_descriptor_pool_size            = VkDescriptorPoolSize();
    uniform_descriptor_pool_size.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    uniform_descriptor_pool_size.descriptorCount = 32;

    auto sampler_descriptor_pool_size            = VkDescriptorPoolSize();
    sampler_descriptor_pool_size.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_descriptor_pool_size.descriptorCount = 32;

    auto const descriptor_pool_sizes = std::array{ uniform_descriptor_pool_size, sampler_descriptor_pool_size };

    auto descriptor_pool_create_info          = VkDescriptorPoolCreateInfo();
    descriptor_pool_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.poolSizeCount = static_cast< uint32_t >( std::size( descriptor_pool_sizes ) );
    descriptor_pool_create_info.pPoolSizes    = std::data( descriptor_pool_sizes );
    descriptor_pool_create_info.maxSets       = 128;
    descriptor_pool_create_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    vkCreateDescriptorPool( ctx.device, &descriptor_pool_create_info, nullptr, &ctx.descriptor_pool );
  }

  void destroy_pools( context & ctx ) noexcept
  {
    vkDestroyDescriptorPool( ctx.device, ctx.descriptor_pool, nullptr );
    vkDestroyCommandPool( ctx.device, ctx.command_pool, nullptr );
  }

  void init_swapchain( context & ctx ) noexcept
  {
    auto const family_indices   = std::array{ ctx.graphics_queue_index, ctx.present_queue_index };
    auto const framebuffer_size = query_framebuffer_size( ctx );

    auto capabilities = VkSurfaceCapabilitiesKHR();
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( ctx.physical_device, ctx.surface, &capabilities );

    auto const present_mode = detail::choose_present_mode( ctx.physical_device, ctx.surface );
    ctx.swapchain_extent    = detail::choose_extent( capabilities, framebuffer_size );
    auto const image_count  = detail::choose_image_count( capabilities );

    auto swapchain_create_info             = VkSwapchainCreateInfoKHR();
    swapchain_create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface          = ctx.surface;
    swapchain_create_info.minImageCount    = image_count;
    swapchain_create_info.imageFormat      = ctx.surface_format.format;
    swapchain_create_info.imageColorSpace  = ctx.surface_format.colorSpace;
    swapchain_create_info.imageExtent      = ctx.swapchain_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_create_info.preTransform     = capabilities.currentTransform;
    swapchain_create_info.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode      = present_mode;
    swapchain_create_info.clipped          = VK_TRUE;
    swapchain_create_info.oldSwapchain     = nullptr;

    if ( family_indices[ 0 ] != family_indices[ 1 ] )
    {
      swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
      swapchain_create_info.queueFamilyIndexCount = 2;
      swapchain_create_info.pQueueFamilyIndices   = std::data( family_indices );
    }
    else
    {
      swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
      swapchain_create_info.queueFamilyIndexCount = 0;
      swapchain_create_info.pQueueFamilyIndices   = nullptr;
    }

    auto result = vkCreateSwapchainKHR( ctx.device, &swapchain_create_info, nullptr, &ctx.swapchain );
    MVK_VERIFY( result == VK_SUCCESS );

    result = vkGetSwapchainImagesKHR( ctx.device, ctx.swapchain, &ctx.swapchain_images_count, nullptr );
    MVK_VERIFY( result == VK_SUCCESS );

    auto swapchain_images = std::vector< VkImage >( ctx.swapchain_images_count );
    vkGetSwapchainImagesKHR( ctx.device, ctx.swapchain, &ctx.swapchain_images_count, std::data( swapchain_images ) );

    ctx.swapchain_image_views.reserve( ctx.swapchain_images_count );

    for ( auto const image : swapchain_images )
    {
      auto swapchain_image_view_create_info                            = VkImageViewCreateInfo();
      swapchain_image_view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      swapchain_image_view_create_info.image                           = image;
      swapchain_image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
      swapchain_image_view_create_info.format                          = swapchain_create_info.imageFormat;
      swapchain_image_view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      swapchain_image_view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      swapchain_image_view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      swapchain_image_view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      swapchain_image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      swapchain_image_view_create_info.subresourceRange.baseMipLevel   = 0;
      swapchain_image_view_create_info.subresourceRange.levelCount     = 1;
      swapchain_image_view_create_info.subresourceRange.baseArrayLayer = 0;
      swapchain_image_view_create_info.subresourceRange.layerCount     = 1;

      auto image_view = VkImageView();
      result          = vkCreateImageView( ctx.device, &swapchain_image_view_create_info, nullptr, &image_view );
      ctx.swapchain_image_views.push_back( image_view );
    }
  }

  void destroy_swapchain( context & ctx ) noexcept
  {
    for ( auto const image_view : ctx.swapchain_image_views )
    {
      vkDestroyImageView( ctx.device, image_view, nullptr );
    }

    vkDestroySwapchainKHR( ctx.device, ctx.swapchain, nullptr );
  }

  void init_depth_image( context & ctx ) noexcept
  {
    auto depth_image_create_info          = VkImageCreateInfo();
    depth_image_create_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depth_image_create_info.imageType     = VK_IMAGE_TYPE_2D;
    depth_image_create_info.extent.width  = ctx.swapchain_extent.width;
    depth_image_create_info.extent.height = ctx.swapchain_extent.height;
    depth_image_create_info.extent.depth  = 1;
    depth_image_create_info.mipLevels     = 1;
    depth_image_create_info.arrayLayers   = 1;
    depth_image_create_info.format        = VK_FORMAT_D32_SFLOAT;
    depth_image_create_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    depth_image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_image_create_info.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depth_image_create_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    depth_image_create_info.samples       = VK_SAMPLE_COUNT_1_BIT;
    depth_image_create_info.flags         = 0;

    auto result = vkCreateImage( ctx.device, &depth_image_create_info, nullptr, &ctx.depth_image );
    MVK_VERIFY( result == VK_SUCCESS );

    auto depth_image_requirements = VkMemoryRequirements();
    vkGetImageMemoryRequirements( ctx.device, ctx.depth_image, &depth_image_requirements );

    auto const memory_type_index = detail::find_memory_type(
      ctx.physical_device, depth_image_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    MVK_VERIFY( memory_type_index.has_value() );

    auto depth_image_memory_allocate_info            = VkMemoryAllocateInfo();
    depth_image_memory_allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    depth_image_memory_allocate_info.allocationSize  = depth_image_requirements.size;
    depth_image_memory_allocate_info.memoryTypeIndex = memory_type_index.value();

    result = vkAllocateMemory( ctx.device, &depth_image_memory_allocate_info, nullptr, &ctx.depth_image_memory );
    MVK_VERIFY( result == VK_SUCCESS );

    result = vkBindImageMemory( ctx.device, ctx.depth_image, ctx.depth_image_memory, 0 );
    MVK_VERIFY( result == VK_SUCCESS );

    auto depth_image_view_create_info                            = VkImageViewCreateInfo();
    depth_image_view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depth_image_view_create_info.image                           = ctx.depth_image;
    depth_image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    depth_image_view_create_info.format                          = VK_FORMAT_D32_SFLOAT;
    depth_image_view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    depth_image_view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    depth_image_view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    depth_image_view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    depth_image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth_image_view_create_info.subresourceRange.baseMipLevel   = 0;
    depth_image_view_create_info.subresourceRange.levelCount     = 1;
    depth_image_view_create_info.subresourceRange.baseArrayLayer = 0;
    depth_image_view_create_info.subresourceRange.layerCount     = 1;

    result = vkCreateImageView( ctx.device, &depth_image_view_create_info, nullptr, &ctx.depth_image_view );

    MVK_VERIFY( result == VK_SUCCESS );

    transition_layout(
      ctx, ctx.depth_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1 );
  }

  void destroy_depth_image( context & ctx ) noexcept
  {
    vkDestroyImageView( ctx.device, ctx.depth_image_view, nullptr );
    vkFreeMemory( ctx.device, ctx.depth_image_memory, nullptr );
    vkDestroyImage( ctx.device, ctx.depth_image, nullptr );
  }

  void init_framebuffers( context & ctx ) noexcept
  {
    ctx.framebuffers.reserve( ctx.swapchain_images_count );

    for ( auto const image_view : ctx.swapchain_image_views )
    {
      auto const attachments = std::array{ image_view, ctx.depth_image_view };

      auto framebuffer_create_info            = VkFramebufferCreateInfo();
      framebuffer_create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer_create_info.renderPass      = ctx.render_pass;
      framebuffer_create_info.attachmentCount = static_cast< uint32_t >( std::size( attachments ) );
      framebuffer_create_info.pAttachments    = std::data( attachments );
      framebuffer_create_info.width           = ctx.swapchain_extent.width;
      framebuffer_create_info.height          = ctx.swapchain_extent.height;
      framebuffer_create_info.layers          = 1;

      auto framebuffer = VkFramebuffer();

      [[maybe_unused]] auto result = vkCreateFramebuffer( ctx.device, &framebuffer_create_info, nullptr, &framebuffer );
      ctx.framebuffers.push_back( framebuffer );
      MVK_VERIFY( result == VK_SUCCESS );
    }
  }

  void destroy_framebuffers( context & ctx ) noexcept
  {
    for ( auto const framebuffer : ctx.framebuffers )
    {
      vkDestroyFramebuffer( ctx.device, framebuffer, nullptr );
    }
  }

  void init_render_pass( context & ctx ) noexcept
  {
    auto color_attachment_description           = VkAttachmentDescription();
    color_attachment_description.format         = ctx.surface_format.format;
    color_attachment_description.samples        = VK_SAMPLE_COUNT_1_BIT;
    color_attachment_description.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment_description.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_description.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment_description.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment_description.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    auto depth_attachment_description           = VkAttachmentDescription();
    depth_attachment_description.format         = VK_FORMAT_D32_SFLOAT;
    depth_attachment_description.samples        = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment_description.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment_description.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_description.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_description.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment_description.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    auto color_attachment_reference       = VkAttachmentReference();
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    auto depth_attachment_reference       = VkAttachmentReference();
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    auto subpass_description                    = VkSubpassDescription();
    subpass_description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.colorAttachmentCount    = 1;
    subpass_description.pColorAttachments       = &color_attachment_reference;
    subpass_description.pDepthStencilAttachment = &depth_attachment_reference;

    auto subpass_dependency          = VkSubpassDependency();
    subpass_dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass    = 0;
    subpass_dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    auto const attachments = std::array{ color_attachment_description, depth_attachment_description };

    auto render_pass_create_info            = VkRenderPassCreateInfo();
    render_pass_create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = static_cast< uint32_t >( std::size( attachments ) );
    render_pass_create_info.pAttachments    = std::data( attachments );
    render_pass_create_info.subpassCount    = 1;
    render_pass_create_info.pSubpasses      = &subpass_description;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies   = &subpass_dependency;

    [[maybe_unused]] auto result =
      vkCreateRenderPass( ctx.device, &render_pass_create_info, nullptr, &ctx.render_pass );
    MVK_VERIFY( result == VK_SUCCESS );
  }

  void destroy_render_pass( context & ctx ) noexcept
  {
    vkDestroyRenderPass( ctx.device, ctx.render_pass, nullptr );
  }

  void init_doesnt_belong_here( context & ctx ) noexcept
  {
    std::tie( ctx.texture_, ctx.width_, ctx.height_ ) = detail::load_texture( "../../assets/viking_room.png" );

    auto image_create_info          = VkImageCreateInfo();
    image_create_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType     = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width  = ctx.width_;
    image_create_info.extent.height = ctx.height_;
    image_create_info.extent.depth  = 1;
    image_create_info.mipLevels     = detail::calculate_mimap_levels( ctx.width_, ctx.height_ );
    image_create_info.arrayLayers   = 1;
    image_create_info.format        = VK_FORMAT_R8G8B8A8_SRGB;
    image_create_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage =
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.samples     = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.flags       = 0;

    auto result = vkCreateImage( ctx.device, &image_create_info, nullptr, &ctx.image_ );
    MVK_VERIFY( result == VK_SUCCESS );

    auto image_requirements = VkMemoryRequirements();
    vkGetImageMemoryRequirements( ctx.device, ctx.image_, &image_requirements );

    auto const memory_type_index = detail::find_memory_type(
      ctx.physical_device, image_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    MVK_VERIFY( memory_type_index.has_value() );

    auto image_memory_allocate_info            = VkMemoryAllocateInfo();
    image_memory_allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    image_memory_allocate_info.allocationSize  = image_requirements.size;
    image_memory_allocate_info.memoryTypeIndex = memory_type_index.value();

    result = vkAllocateMemory( ctx.device, &image_memory_allocate_info, nullptr, &ctx.image_memory_ );
    MVK_VERIFY( result == VK_SUCCESS );

    vkBindImageMemory( ctx.device, ctx.image_, ctx.image_memory_, 0 );

    transition_layout(
      ctx, ctx.image_, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image_create_info.mipLevels );

    auto staged_texture = staging_allocate( ctx, utility::as_bytes( ctx.texture_ ) );
    stage_image( ctx, staged_texture, ctx.width_, ctx.height_, ctx.image_ );
    generate_mipmaps( ctx, ctx.image_, ctx.width_, ctx.height_, image_create_info.mipLevels );

    auto image_view_create_info                            = VkImageViewCreateInfo();
    image_view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image                           = ctx.image_;
    image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format                          = VK_FORMAT_R8G8B8A8_SRGB;
    image_view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel   = 0;
    image_view_create_info.subresourceRange.levelCount     = image_create_info.mipLevels;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount     = 1;

    result = vkCreateImageView( ctx.device, &image_view_create_info, nullptr, &ctx.image_view_ );
    MVK_VERIFY( result == VK_SUCCESS );

    std::tie( ctx.vertices_, ctx.indices_ ) = detail::read_object( "../../assets/viking_room.obj" );

    ctx.image_descriptor_set_ = allocate_descriptor_sets< 1 >( ctx, ctx.texture_descriptor_set_layout )[ 0 ];

    auto image_descriptor_image_info        = VkDescriptorImageInfo();
    image_descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_descriptor_image_info.imageView   = ctx.image_view_;
    image_descriptor_image_info.sampler     = ctx.texture_sampler;

    auto image_write_descriptor_set             = VkWriteDescriptorSet();
    image_write_descriptor_set.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    image_write_descriptor_set.dstSet           = ctx.image_descriptor_set_;
    image_write_descriptor_set.dstBinding       = 0;
    image_write_descriptor_set.dstArrayElement  = 0;
    image_write_descriptor_set.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    image_write_descriptor_set.descriptorCount  = 1;
    image_write_descriptor_set.pBufferInfo      = nullptr;
    image_write_descriptor_set.pImageInfo       = &image_descriptor_image_info;
    image_write_descriptor_set.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets( ctx.device, 1, &image_write_descriptor_set, 0, nullptr );
  }

  void destroy_doesnt_beling_here( context & ctx ) noexcept
  {
    vkFreeDescriptorSets( ctx.device, ctx.descriptor_pool, 1, &ctx.image_descriptor_set_ );
    vkDestroyImageView( ctx.device, ctx.image_view_, nullptr );
    vkFreeMemory( ctx.device, ctx.image_memory_, nullptr );
    vkDestroyImage( ctx.device, ctx.image_, nullptr );
  }

  void init_command_buffers( context & ctx ) noexcept
  {
    ctx.command_buffers =
      allocate_command_buffers< context::dynamic_buffer_count >( ctx, VK_COMMAND_BUFFER_LEVEL_PRIMARY );
  }

  void destroy_command_buffers( context & ctx ) noexcept
  {
    vkFreeCommandBuffers( ctx.device,
                          ctx.command_pool,
                          static_cast< uint32_t >( std::size( ctx.command_buffers ) ),
                          std::data( ctx.command_buffers ) );
  }

  void init_shaders( context & ctx ) noexcept
  {
    auto const vertex_code = detail::read_file( "../../shaders/vert.spv" );

    auto vertex_shader_module_create_info     = VkShaderModuleCreateInfo();
    vertex_shader_module_create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_module_create_info.codeSize = static_cast< uint32_t >( std::size( vertex_code ) );
    vertex_shader_module_create_info.pCode    = reinterpret_cast< uint32_t const * >( std::data( vertex_code ) );

    auto result = vkCreateShaderModule( ctx.device, &vertex_shader_module_create_info, nullptr, &ctx.vertex_shader );
    MVK_VERIFY( result == VK_SUCCESS );

    auto const fragment_code = detail::read_file( "../../shaders/frag.spv" );

    auto fragment_shader_module_create_info     = VkShaderModuleCreateInfo();
    fragment_shader_module_create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragment_shader_module_create_info.codeSize = static_cast< uint32_t >( std::size( fragment_code ) );
    fragment_shader_module_create_info.pCode    = reinterpret_cast< uint32_t const * >( std::data( fragment_code ) );

    result = vkCreateShaderModule( ctx.device, &fragment_shader_module_create_info, nullptr, &ctx.fragment_shader );
    MVK_VERIFY( result == VK_SUCCESS );
  }

  void destroy_shaders( context & ctx ) noexcept
  {
    vkDestroyShaderModule( ctx.device, ctx.fragment_shader, nullptr );
    vkDestroyShaderModule( ctx.device, ctx.vertex_shader, nullptr );
  }

  void init_samplers( context & ctx ) noexcept
  {
    auto sampler_create_info                    = VkSamplerCreateInfo();
    sampler_create_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter               = VK_FILTER_LINEAR;
    sampler_create_info.minFilter               = VK_FILTER_LINEAR;
    sampler_create_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.anisotropyEnable        = VK_TRUE;
    sampler_create_info.anisotropyEnable        = VK_TRUE;
    sampler_create_info.maxAnisotropy           = 16;
    sampler_create_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;
    sampler_create_info.compareEnable           = VK_FALSE;
    sampler_create_info.compareOp               = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.mipLodBias              = 0.0F;
    sampler_create_info.minLod                  = 0.0F;
    sampler_create_info.maxLod                  = std::numeric_limits< float >::max();

    [[maybe_unused]] auto result = vkCreateSampler( ctx.device, &sampler_create_info, nullptr, &ctx.texture_sampler );
    MVK_VERIFY( result == VK_SUCCESS );
  }

  void destroy_samplers( context & ctx ) noexcept
  {
    vkDestroySampler( ctx.device, ctx.texture_sampler, nullptr );
  }

  void init_pipeline( context & ctx ) noexcept
  {
    auto vertex_input_binding_description      = VkVertexInputBindingDescription();
    vertex_input_binding_description.binding   = 0;
    vertex_input_binding_description.stride    = sizeof( vertex );
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    auto position_vertex_input_attribute_description     = VkVertexInputAttributeDescription();
    position_vertex_input_attribute_description.binding  = 0;
    position_vertex_input_attribute_description.location = 0;
    position_vertex_input_attribute_description.format   = VK_FORMAT_R32G32B32_SFLOAT;
    position_vertex_input_attribute_description.offset   = offsetof( vertex, pos );

    auto color_vertex_input_attribute_description     = VkVertexInputAttributeDescription();
    color_vertex_input_attribute_description.binding  = 0;
    color_vertex_input_attribute_description.location = 1;
    color_vertex_input_attribute_description.format   = VK_FORMAT_R32G32B32_SFLOAT;
    color_vertex_input_attribute_description.offset   = offsetof( vertex, color );

    auto texture_coordinate_vertex_input_attribute_description     = VkVertexInputAttributeDescription();
    texture_coordinate_vertex_input_attribute_description.binding  = 0;
    texture_coordinate_vertex_input_attribute_description.location = 2;
    texture_coordinate_vertex_input_attribute_description.format   = VK_FORMAT_R32G32_SFLOAT;
    texture_coordinate_vertex_input_attribute_description.offset   = offsetof( vertex, texture_coord );

    auto const vertex_attributes = std::array{ position_vertex_input_attribute_description,
                                               color_vertex_input_attribute_description,
                                               texture_coordinate_vertex_input_attribute_description };

    auto pipeline_vertex_input_state_create_info  = VkPipelineVertexInputStateCreateInfo();
    pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    pipeline_vertex_input_state_create_info.pVertexBindingDescriptions    = &vertex_input_binding_description;
    pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount =
      static_cast< uint32_t >( std::size( vertex_attributes ) );
    pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = std::data( vertex_attributes );

    auto pipeline_input_assembly_state_create_info     = VkPipelineInputAssemblyStateCreateInfo();
    pipeline_input_assembly_state_create_info.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    auto viewport     = VkViewport();
    viewport.x        = 0.0F;
    viewport.y        = 0.0F;
    viewport.width    = static_cast< float >( ctx.swapchain_extent.width );
    viewport.height   = static_cast< float >( ctx.swapchain_extent.height );
    viewport.minDepth = 0.0F;
    viewport.maxDepth = 1.0F;

    auto scissor     = VkRect2D();
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent   = ctx.swapchain_extent;

    auto pipeline_viewport_state_create_info          = VkPipelineViewportStateCreateInfo();
    pipeline_viewport_state_create_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipeline_viewport_state_create_info.viewportCount = 1;
    pipeline_viewport_state_create_info.pViewports    = &viewport;
    pipeline_viewport_state_create_info.scissorCount  = 1;
    pipeline_viewport_state_create_info.pScissors     = &scissor;

    auto pipeline_rasterization_state_create_info  = VkPipelineRasterizationStateCreateInfo();
    pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipeline_rasterization_state_create_info.depthClampEnable        = VK_FALSE;
    pipeline_rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    pipeline_rasterization_state_create_info.polygonMode             = VK_POLYGON_MODE_FILL;
    pipeline_rasterization_state_create_info.lineWidth               = 1.0F;
    pipeline_rasterization_state_create_info.cullMode                = VK_CULL_MODE_BACK_BIT;
    pipeline_rasterization_state_create_info.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipeline_rasterization_state_create_info.depthBiasEnable         = VK_FALSE;
    pipeline_rasterization_state_create_info.depthBiasConstantFactor = 0.0F;
    pipeline_rasterization_state_create_info.depthBiasClamp          = 0.0F;
    pipeline_rasterization_state_create_info.depthBiasSlopeFactor    = 0.0F;

    auto pipeline_multisample_state_create_info  = VkPipelineMultisampleStateCreateInfo();
    pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipeline_multisample_state_create_info.sampleShadingEnable   = VK_FALSE;
    pipeline_multisample_state_create_info.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    pipeline_multisample_state_create_info.minSampleShading      = 1.0F;
    pipeline_multisample_state_create_info.pSampleMask           = nullptr;
    pipeline_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    pipeline_multisample_state_create_info.alphaToOneEnable      = VK_FALSE;

    auto pipeline_color_blend_attachment = VkPipelineColorBlendAttachmentState();
    pipeline_color_blend_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    pipeline_color_blend_attachment.blendEnable         = VK_FALSE;
    pipeline_color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    pipeline_color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipeline_color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
    pipeline_color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    pipeline_color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipeline_color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;

    auto pipeline_color_blend_state_create_info            = VkPipelineColorBlendStateCreateInfo();
    pipeline_color_blend_state_create_info.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipeline_color_blend_state_create_info.logicOpEnable   = VK_FALSE;
    pipeline_color_blend_state_create_info.logicOp         = VK_LOGIC_OP_COPY;
    pipeline_color_blend_state_create_info.attachmentCount = 1;
    pipeline_color_blend_state_create_info.pAttachments    = &pipeline_color_blend_attachment;
    pipeline_color_blend_state_create_info.blendConstants[ 0 ] = 0.0F;
    pipeline_color_blend_state_create_info.blendConstants[ 1 ] = 0.0F;
    pipeline_color_blend_state_create_info.blendConstants[ 2 ] = 0.0F;
    pipeline_color_blend_state_create_info.blendConstants[ 3 ] = 0.0F;

    auto pipeline_depth_stencil_state_create_info  = VkPipelineDepthStencilStateCreateInfo();
    pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipeline_depth_stencil_state_create_info.depthTestEnable       = VK_TRUE;
    pipeline_depth_stencil_state_create_info.depthWriteEnable      = VK_TRUE;
    pipeline_depth_stencil_state_create_info.depthCompareOp        = VK_COMPARE_OP_LESS;
    pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    pipeline_depth_stencil_state_create_info.minDepthBounds        = 0.0F;
    pipeline_depth_stencil_state_create_info.maxDepthBounds        = 1.0F;
    pipeline_depth_stencil_state_create_info.stencilTestEnable     = VK_FALSE;

    auto vertex_pipeline_shader_stage_create_info   = VkPipelineShaderStageCreateInfo();
    vertex_pipeline_shader_stage_create_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_pipeline_shader_stage_create_info.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_pipeline_shader_stage_create_info.module = ctx.vertex_shader;
    vertex_pipeline_shader_stage_create_info.pName  = "main";

    auto fragment_pipeline_shader_stage_create_info   = VkPipelineShaderStageCreateInfo();
    fragment_pipeline_shader_stage_create_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_pipeline_shader_stage_create_info.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_pipeline_shader_stage_create_info.module = ctx.fragment_shader;
    fragment_pipeline_shader_stage_create_info.pName  = "main";

    auto const shader_stages =
      std::array{ vertex_pipeline_shader_stage_create_info, fragment_pipeline_shader_stage_create_info };

    auto pipeline_create_info                = VkGraphicsPipelineCreateInfo();
    pipeline_create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount          = static_cast< uint32_t >( std::size( shader_stages ) );
    pipeline_create_info.pStages             = std::data( shader_stages );
    pipeline_create_info.pVertexInputState   = &pipeline_vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
    pipeline_create_info.pViewportState      = &pipeline_viewport_state_create_info;
    pipeline_create_info.pRasterizationState = &pipeline_rasterization_state_create_info;
    pipeline_create_info.pMultisampleState   = &pipeline_multisample_state_create_info;
    pipeline_create_info.pDepthStencilState  = &pipeline_depth_stencil_state_create_info;
    pipeline_create_info.pColorBlendState    = &pipeline_color_blend_state_create_info;
    pipeline_create_info.pDynamicState       = nullptr;
    pipeline_create_info.layout              = ctx.pipeline_layout;
    pipeline_create_info.renderPass          = ctx.render_pass;
    pipeline_create_info.subpass             = 0;
    pipeline_create_info.basePipelineHandle  = nullptr;
    pipeline_create_info.basePipelineIndex   = -1;

    [[maybe_unused]] auto result =
      vkCreateGraphicsPipelines( ctx.device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &ctx.pipeline );
    MVK_VERIFY( result == VK_SUCCESS );
  }

  void destroy_pipelines( context & ctx ) noexcept
  {
    vkDestroyPipeline( ctx.device, ctx.pipeline, nullptr );
  }

  void init_sync( context & ctx ) noexcept
  {
    auto semaphore_create_info  = VkSemaphoreCreateInfo();
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    auto fence_create_info  = VkFenceCreateInfo();
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for ( auto i = size_t( 0 ); i < context::max_frames_in_flight; ++i )
    {
      auto result =
        vkCreateSemaphore( ctx.device, &semaphore_create_info, nullptr, &ctx.image_available_semaphores[ i ] );
      MVK_VERIFY( result == VK_SUCCESS );

      result = vkCreateSemaphore( ctx.device, &semaphore_create_info, nullptr, &ctx.render_finished_semaphores[ i ] );
      MVK_VERIFY( result == VK_SUCCESS );

      result = vkCreateFence( ctx.device, &fence_create_info, nullptr, &ctx.frame_in_flight_fences[ i ] );
      MVK_VERIFY( result == VK_SUCCESS );
    }

    ctx.image_in_flight_fences.resize( ctx.swapchain_images_count, nullptr );
  }

  void destroy_sync( context & ctx ) noexcept
  {
    for ( auto i = size_t( 0 ); i < context::max_frames_in_flight; ++i )
    {
      vkDestroySemaphore( ctx.device, ctx.image_available_semaphores[ i ], nullptr );
      vkDestroySemaphore( ctx.device, ctx.render_finished_semaphores[ i ], nullptr );
      vkDestroyFence( ctx.device, ctx.frame_in_flight_fences[ i ], nullptr );
    }
  }

}  // namespace mvk::engine
