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

    auto application_info = [ &name ]
    {
      auto info               = VkApplicationInfo();
      info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      info.pApplicationName   = name;
      info.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
      info.pEngineName        = "No Engine";
      info.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );
      return info;
    }();

    auto       count = uint32_t( 0 );
    auto const data  = glfwGetRequiredInstanceExtensions( &count );

    auto required_extensions = std::vector< char const * >( data, std::next( data, count ) );

    if constexpr ( context::use_validation )
    {
      required_extensions.insert( std::begin( required_extensions ),
                                  std::begin( context::validation_instance_extensions ),
                                  std::end( context::validation_instance_extensions ) );
    }

    auto const instance_create_info = [ &required_extensions, &application_info ]
    {
      auto info  = VkInstanceCreateInfo();
      info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

      if constexpr ( context::use_validation )
      {
        info.pNext = &debug_create_info;
      }
      else
      {
        info.pNext = nullptr;
      }

      info.pApplicationInfo = &application_info;

      if constexpr ( context::use_validation )
      {
        info.enabledLayerCount   = static_cast< uint32_t >( std::size( context::validation_layers ) );
        info.ppEnabledLayerNames = std::data( context::validation_layers );
      }
      else
      {
        info.enabledLayerCount       = 0;
        info.ppEnabledExtensionNames = nullptr;
      }

      info.enabledExtensionCount   = static_cast< uint32_t >( std::size( required_extensions ) );
      info.ppEnabledExtensionNames = std::data( required_extensions );
      return info;
    }();

    auto result = vkCreateInstance( &instance_create_info, nullptr, &ctx.instance );
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

    auto const graphics_queue_create_info = [ &queue_priority, &ctx ]
    {
      auto info             = VkDeviceQueueCreateInfo();
      info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      info.queueFamilyIndex = ctx.graphics_queue_index;
      info.queueCount       = 1;
      info.pQueuePriorities = &queue_priority;
      return info;
    }();

    auto const present_queue_create_info = [ &queue_priority, &ctx ]
    {
      auto info             = VkDeviceQueueCreateInfo();
      info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      info.queueFamilyIndex = ctx.present_queue_index;
      info.queueCount       = 1;
      info.pQueuePriorities = &queue_priority;
      return info;
    }();

    auto const queue_create_info       = std::array{ graphics_queue_create_info, present_queue_create_info };
    auto const queue_create_info_count = static_cast< uint32_t >( queue_indices.first != queue_indices.second ? 2 : 1 );

    auto const device_create_info = [ &queue_create_info, queue_create_info_count, &features ]
    {
      auto info                    = VkDeviceCreateInfo();
      info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      info.queueCreateInfoCount    = queue_create_info_count;
      info.pQueueCreateInfos       = std::data( queue_create_info );
      info.pEnabledFeatures        = &features;
      info.enabledExtensionCount   = static_cast< uint32_t >( std::size( context::device_extensions ) );
      info.ppEnabledExtensionNames = std::data( context::device_extensions );

      if constexpr ( context::use_validation )
      {
        info.enabledLayerCount   = static_cast< uint32_t >( std::size( context::validation_layers ) );
        info.ppEnabledLayerNames = std::data( context::validation_layers );
      }
      else
      {
        info.enabledLayerCount   = 0;
        info.ppEnabledLayerNames = nullptr;
      }

      return info;
    }();

    auto result = vkCreateDevice( ctx.physical_device, &device_create_info, nullptr, &ctx.device );
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

    auto const uniform_descriptor_set_layout_create_info = [ &uniform_descriptor_set_bindings ]
    {
      auto info         = VkDescriptorSetLayoutCreateInfo();
      info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      info.bindingCount = static_cast< uint32_t >( std::size( uniform_descriptor_set_bindings ) );
      info.pBindings    = std::data( uniform_descriptor_set_bindings );
      return info;
    }();

    auto uniform_layout_result = vkCreateDescriptorSetLayout(
      ctx.device, &uniform_descriptor_set_layout_create_info, nullptr, &ctx.uniform_descriptor_set_layout );

    MVK_VERIFY( uniform_layout_result == VK_SUCCESS );

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

    auto const sampler_descriptor_set_layout_create_info = [ &sampler_descriptor_set_bindings ]
    {
      auto info         = VkDescriptorSetLayoutCreateInfo();
      info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      info.bindingCount = static_cast< uint32_t >( std::size( sampler_descriptor_set_bindings ) );
      info.pBindings    = std::data( sampler_descriptor_set_bindings );
      return info;
    }();

    auto sampler_layout_result = vkCreateDescriptorSetLayout(
      ctx.device, &sampler_descriptor_set_layout_create_info, nullptr, &ctx.texture_descriptor_set_layout );

    MVK_VERIFY( sampler_layout_result == VK_SUCCESS );

    auto descriptor_set_layouts = std::array{ ctx.uniform_descriptor_set_layout, ctx.texture_descriptor_set_layout };

    auto const pipeline_layout_create_info = [ &descriptor_set_layouts ]
    {
      auto info                   = VkPipelineLayoutCreateInfo();
      info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
      info.setLayoutCount         = static_cast< uint32_t >( std::size( descriptor_set_layouts ) );
      info.pSetLayouts            = std::data( descriptor_set_layouts );
      info.pushConstantRangeCount = 0;
      info.pPushConstantRanges    = nullptr;
      return info;
    }();

    auto pipeline_layout_result =
      vkCreatePipelineLayout( ctx.device, &pipeline_layout_create_info, nullptr, &ctx.pipeline_layout );

    MVK_VERIFY( pipeline_layout_result == VK_SUCCESS );
  }

  void destroy_layouts( context & ctx ) noexcept
  {
    vkDestroyPipelineLayout( ctx.device, ctx.pipeline_layout, nullptr );
    vkDestroyDescriptorSetLayout( ctx.device, ctx.texture_descriptor_set_layout, nullptr );
    vkDestroyDescriptorSetLayout( ctx.device, ctx.uniform_descriptor_set_layout, nullptr );
  }

  void init_pools( context & ctx ) noexcept
  {
    auto const command_pool_create_info = [ &ctx ]
    {
      auto info             = VkCommandPoolCreateInfo();
      info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      info.queueFamilyIndex = ctx.graphics_queue_index;
      info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      return info;
    }();

    auto command_pool_result = vkCreateCommandPool( ctx.device, &command_pool_create_info, nullptr, &ctx.command_pool );
    MVK_VERIFY( command_pool_result == VK_SUCCESS );

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

    auto const descriptor_pool_create_info = [ &descriptor_pool_sizes ]
    {
      auto info          = VkDescriptorPoolCreateInfo();
      info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      info.poolSizeCount = static_cast< uint32_t >( std::size( descriptor_pool_sizes ) );
      info.pPoolSizes    = std::data( descriptor_pool_sizes );
      info.maxSets       = 128;
      info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
      return info;
    }();

    vkCreateDescriptorPool( ctx.device, &descriptor_pool_create_info, nullptr, &ctx.descriptor_pool );
  }

  void destroy_pools( context & ctx ) noexcept
  {
    vkDestroyDescriptorPool( ctx.device, ctx.descriptor_pool, nullptr );
    vkDestroyCommandPool( ctx.device, ctx.command_pool, nullptr );
  }

  void init_swapchain( context & ctx ) noexcept
  {
    auto const family_indices = std::array{ ctx.graphics_queue_index, ctx.present_queue_index };

    auto const swapchain_create_info = [ &ctx, &family_indices ]
    {
      auto const framebuffer_size = query_framebuffer_size( ctx );

      auto capabilities = VkSurfaceCapabilitiesKHR();
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR( ctx.physical_device, ctx.surface, &capabilities );

      auto const present_mode = detail::choose_present_mode( ctx.physical_device, ctx.surface );
      ctx.swapchain_extent    = detail::choose_extent( capabilities, framebuffer_size );
      auto const image_count  = detail::choose_image_count( capabilities );

      auto info             = VkSwapchainCreateInfoKHR();
      info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      info.surface          = ctx.surface;
      info.minImageCount    = image_count;
      info.imageFormat      = ctx.surface_format.format;
      info.imageColorSpace  = ctx.surface_format.colorSpace;
      info.imageExtent      = ctx.swapchain_extent;
      info.imageArrayLayers = 1;
      info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      info.preTransform     = capabilities.currentTransform;
      info.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
      info.presentMode      = present_mode;
      info.clipped          = VK_TRUE;
      info.oldSwapchain     = nullptr;

      if ( family_indices[ 0 ] != family_indices[ 1 ] )
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

    auto swapchain_result = vkCreateSwapchainKHR( ctx.device, &swapchain_create_info, nullptr, &ctx.swapchain );
    MVK_VERIFY( swapchain_result == VK_SUCCESS );

    vkGetSwapchainImagesKHR( ctx.device, ctx.swapchain, &ctx.swapchain_images_count, nullptr );

    auto swapchain_images = std::vector< VkImage >( ctx.swapchain_images_count );
    vkGetSwapchainImagesKHR( ctx.device, ctx.swapchain, &ctx.swapchain_images_count, std::data( swapchain_images ) );

    ctx.swapchain_image_views.reserve( ctx.swapchain_images_count );

    auto const add_image_view = [ &ctx, &swapchain_create_info ]( auto const image )
    {
      auto const image_view_create_info = [ image, &swapchain_create_info ]
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

      auto image_view = VkImageView();

      auto result = vkCreateImageView( ctx.device, &image_view_create_info, nullptr, &image_view );
      MVK_VERIFY( result == VK_SUCCESS );

      ctx.swapchain_image_views.push_back( image_view );
    };

    std::for_each( std::begin( swapchain_images ), std::end( swapchain_images ), add_image_view );
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
    auto const depth_image_create_info = [ &ctx ]
    {
      auto info          = VkImageCreateInfo();
      info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      info.imageType     = VK_IMAGE_TYPE_2D;
      info.extent.width  = ctx.swapchain_extent.width;
      info.extent.height = ctx.swapchain_extent.height;
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

    auto depth_image_result = vkCreateImage( ctx.device, &depth_image_create_info, nullptr, &ctx.depth_image );
    MVK_VERIFY( depth_image_result == VK_SUCCESS );

    auto depth_image_requirements = VkMemoryRequirements();
    vkGetImageMemoryRequirements( ctx.device, ctx.depth_image, &depth_image_requirements );

    auto const memory_type_index = detail::find_memory_type(
      ctx.physical_device, depth_image_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    MVK_VERIFY( memory_type_index.has_value() );

    auto depth_image_memory_allocate_info = [ depth_image_requirements, memory_type_index ]
    {
      auto info            = VkMemoryAllocateInfo();
      info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      info.allocationSize  = depth_image_requirements.size;
      info.memoryTypeIndex = memory_type_index.value();
      return info;
    }();

    auto depth_image_memory_result =
      vkAllocateMemory( ctx.device, &depth_image_memory_allocate_info, nullptr, &ctx.depth_image_memory );

    MVK_VERIFY( depth_image_memory_result == VK_SUCCESS );

    vkBindImageMemory( ctx.device, ctx.depth_image, ctx.depth_image_memory, 0 );

    auto const depth_image_view_create_info = [ &ctx ]
    {
      auto info                            = VkImageViewCreateInfo();
      info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      info.image                           = ctx.depth_image;
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

    auto depth_image_view_result =
      vkCreateImageView( ctx.device, &depth_image_view_create_info, nullptr, &ctx.depth_image_view );

    MVK_VERIFY( depth_image_view_result == VK_SUCCESS );

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

    auto const add_framebuffer = [ &ctx ]( auto const & image_view )
    {
      auto const attachments = std::array{ image_view, ctx.depth_image_view };

      auto const framebuffer_create_info = [ &ctx, &attachments ]
      {
        auto info            = VkFramebufferCreateInfo();
        info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass      = ctx.render_pass;
        info.attachmentCount = static_cast< uint32_t >( std::size( attachments ) );
        info.pAttachments    = std::data( attachments );
        info.width           = ctx.swapchain_extent.width;
        info.height          = ctx.swapchain_extent.height;
        info.layers          = 1;
        return info;
      }();

      auto framebuffer = VkFramebuffer();
      auto result      = vkCreateFramebuffer( ctx.device, &framebuffer_create_info, nullptr, &framebuffer );
      ctx.framebuffers.push_back( framebuffer );
      MVK_VERIFY( result == VK_SUCCESS );
    };

    std::for_each( std::begin( ctx.swapchain_image_views ), std::end( ctx.swapchain_image_views ), add_framebuffer );
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
    auto const color_attachment = [ &ctx ]
    {
      auto attachment           = VkAttachmentDescription();
      attachment.format         = ctx.surface_format.format;
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

    auto const subpass_description = [ &color_attachment_reference, &depth_attachment_reference ]
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

    auto const render_pass_create_info = [ &attachments, &subpass_description, &subpass_dependency ]
    {
      auto info            = VkRenderPassCreateInfo();
      info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      info.attachmentCount = static_cast< uint32_t >( std::size( attachments ) );
      info.pAttachments    = std::data( attachments );
      info.subpassCount    = 1;
      info.pSubpasses      = &subpass_description;
      info.dependencyCount = 1;
      info.pDependencies   = &subpass_dependency;
      return info;
    }();

    auto result = vkCreateRenderPass( ctx.device, &render_pass_create_info, nullptr, &ctx.render_pass );
    MVK_VERIFY( result == VK_SUCCESS );
  }

  void destroy_render_pass( context & ctx ) noexcept
  {
    vkDestroyRenderPass( ctx.device, ctx.render_pass, nullptr );
  }

  void init_doesnt_belong_here( context & ctx ) noexcept
  {
    std::tie( ctx.texture_, ctx.width_, ctx.height_ ) = detail::load_texture( "../../assets/viking_room.png" );

    auto const image_create_info = [ &ctx ]
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
      info.usage         = static_cast< VkFlags >( usage );
      info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
      info.samples       = VK_SAMPLE_COUNT_1_BIT;
      info.flags         = 0;
      return info;
    }();

    auto result = vkCreateImage( ctx.device, &image_create_info, nullptr, &ctx.image_ );
    MVK_VERIFY( result == VK_SUCCESS );

    auto image_requirements = VkMemoryRequirements();
    vkGetImageMemoryRequirements( ctx.device, ctx.image_, &image_requirements );

    auto const memory_type_index = detail::find_memory_type(
      ctx.physical_device, image_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    MVK_VERIFY( memory_type_index.has_value() );

    auto image_memory_allocate_info = [ image_requirements, memory_type_index ]
    {
      auto info            = VkMemoryAllocateInfo();
      info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      info.allocationSize  = image_requirements.size;
      info.memoryTypeIndex = memory_type_index.value();
      return info;
    }();

    result = vkAllocateMemory( ctx.device, &image_memory_allocate_info, nullptr, &ctx.image_memory_ );
    MVK_VERIFY( result == VK_SUCCESS );

    vkBindImageMemory( ctx.device, ctx.image_, ctx.image_memory_, 0 );

    transition_layout(
      ctx, ctx.image_, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image_create_info.mipLevels );

    auto staged_texture = staging_allocate( ctx, utility::as_bytes( ctx.texture_ ) );
    stage_image( ctx, staged_texture, ctx.width_, ctx.height_, ctx.image_ );
    generate_mipmaps( ctx, ctx.image_, ctx.width_, ctx.height_, image_create_info.mipLevels );

    auto const image_view_create_info = [ &ctx, &image_create_info ]
    {
      auto info                            = VkImageViewCreateInfo();
      info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      info.image                           = ctx.image_;
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

    result = vkCreateImageView( ctx.device, &image_view_create_info, nullptr, &ctx.image_view_ );
    MVK_VERIFY( result == VK_SUCCESS );

    std::tie( ctx.vertices_, ctx.indices_ ) = detail::read_object( "../../assets/viking_room.obj" );

    ctx.image_descriptor_set_ = allocate_descriptor_sets< 1 >( ctx, ctx.texture_descriptor_set_layout )[ 0 ];

    auto const image_descriptor_image_info = [ &ctx ]
    {
      auto info        = VkDescriptorImageInfo();
      info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      info.imageView   = ctx.image_view_;
      info.sampler     = ctx.texture_sampler;
      return info;
    }();

    auto const image_write = [ &ctx, &image_descriptor_image_info ]
    {
      auto write             = VkWriteDescriptorSet();
      write.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet           = ctx.image_descriptor_set_;
      write.dstBinding       = 0;
      write.dstArrayElement  = 0;
      write.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      write.descriptorCount  = 1;
      write.pBufferInfo      = nullptr;
      write.pImageInfo       = &image_descriptor_image_info;
      write.pTexelBufferView = nullptr;
      return write;
    }();

    vkUpdateDescriptorSets( ctx.device, 1, &image_write, 0, nullptr );
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

    auto const vertex_shader_create_info = [ &vertex_code ]
    {
      auto info     = VkShaderModuleCreateInfo();
      info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      info.codeSize = static_cast< uint32_t >( std::size( vertex_code ) );
      info.pCode    = reinterpret_cast< uint32_t const * >( std::data( vertex_code ) );
      return info;
    }();

    auto result = vkCreateShaderModule( ctx.device, &vertex_shader_create_info, nullptr, &ctx.vertex_shader );
    MVK_VERIFY( result == VK_SUCCESS );

    auto const fragment_code = detail::read_file( "../../shaders/frag.spv" );

    auto const fragment_shader_create_info = [ &fragment_code ]
    {
      auto info     = VkShaderModuleCreateInfo();
      info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      info.codeSize = static_cast< uint32_t >( std::size( fragment_code ) );
      info.pCode    = reinterpret_cast< uint32_t const * >( std::data( fragment_code ) );
      return info;
    }();

    result = vkCreateShaderModule( ctx.device, &fragment_shader_create_info, nullptr, &ctx.fragment_shader );
    MVK_VERIFY( result == VK_SUCCESS );
  }

  void destroy_shaders( context & ctx ) noexcept
  {
    vkDestroyShaderModule( ctx.device, ctx.fragment_shader, nullptr );
    vkDestroyShaderModule( ctx.device, ctx.vertex_shader, nullptr );
  }

  void init_samplers( context & ctx ) noexcept
  {
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
      info.maxLod                  = std::numeric_limits< float >::max();
      return info;
    }();

    auto result = vkCreateSampler( ctx.device, &sampler_create_info, nullptr, &ctx.texture_sampler );
    MVK_VERIFY( result == VK_SUCCESS );
  }

  void destroy_samplers( context & ctx ) noexcept
  {
    vkDestroySampler( ctx.device, ctx.texture_sampler, nullptr );
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

    auto const vertex_input_create_info = [ &vertex_input_binding_description, &vertex_attributes ]
    {
      auto info                            = VkPipelineVertexInputStateCreateInfo();
      info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
      info.vertexBindingDescriptionCount   = 1;
      info.pVertexBindingDescriptions      = &vertex_input_binding_description;
      info.vertexAttributeDescriptionCount = static_cast< uint32_t >( std::size( vertex_attributes ) );
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

    auto const viewport = [ &ctx ]
    {
      auto tmp     = VkViewport();
      tmp.x        = 0.0F;
      tmp.y        = 0.0F;
      tmp.width    = static_cast< float >( ctx.swapchain_extent.width );
      tmp.height   = static_cast< float >( ctx.swapchain_extent.height );
      tmp.minDepth = 0.0F;
      tmp.maxDepth = 1.0F;
      return tmp;
    }();

    auto const scissor = [ &ctx ]
    {
      auto tmp     = VkRect2D();
      tmp.offset.x = 0;
      tmp.offset.y = 0;
      tmp.extent   = ctx.swapchain_extent;
      return tmp;
    }();

    auto const viewport_state_create_info = [ &viewport, &scissor ]
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

    auto const color_blend_create_info = [ &color_blend_attachment ]
    {
      auto info                = VkPipelineColorBlendStateCreateInfo();
      info.sType               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
      info.logicOpEnable       = VK_FALSE;
      info.logicOp             = VK_LOGIC_OP_COPY;
      info.attachmentCount     = 1;
      info.pAttachments        = &color_blend_attachment;
      info.blendConstants[ 0 ] = 0.0F;
      info.blendConstants[ 1 ] = 0.0F;
      info.blendConstants[ 2 ] = 0.0F;
      info.blendConstants[ 3 ] = 0.0F;
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

    auto const vertex_shader_stage = [ &ctx ]
    {
      auto info   = VkPipelineShaderStageCreateInfo();
      info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      info.stage  = VK_SHADER_STAGE_VERTEX_BIT;
      info.module = ctx.vertex_shader;
      info.pName  = "main";
      return info;
    }();

    auto const fragment_shader_stage = [ &ctx ]
    {
      auto info   = VkPipelineShaderStageCreateInfo();
      info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      info.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
      info.module = ctx.fragment_shader;
      info.pName  = "main";
      return info;
    }();

    auto const shader_stages = std::array{ vertex_shader_stage, fragment_shader_stage };

    auto const pipeline_create_info = [ & ]
    {
      auto info                = VkGraphicsPipelineCreateInfo();
      info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
      info.stageCount          = static_cast< uint32_t >( std::size( shader_stages ) );
      info.pStages             = std::data( shader_stages );
      info.pVertexInputState   = &vertex_input_create_info;
      info.pInputAssemblyState = &input_assembly_create_info;
      info.pViewportState      = &viewport_state_create_info;
      info.pRasterizationState = &rasterizer_create_info;
      info.pMultisampleState   = &multisampling_create_info;
      info.pDepthStencilState  = &depth_stencil_state_create_info;
      info.pColorBlendState    = &color_blend_create_info;
      info.pDynamicState       = nullptr;
      info.layout              = ctx.pipeline_layout;
      info.renderPass          = ctx.render_pass;
      info.subpass             = 0;
      info.basePipelineHandle  = nullptr;
      info.basePipelineIndex   = -1;
      return info;
    }();

    auto result =
      vkCreateGraphicsPipelines( ctx.device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &ctx.pipeline );
    MVK_VERIFY( result == VK_SUCCESS );
  }

  void destroy_pipelines( context & ctx ) noexcept
  {
    vkDestroyPipeline( ctx.device, ctx.pipeline, nullptr );
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
