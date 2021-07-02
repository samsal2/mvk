#include "detail/creators.hpp"

namespace mvk::detail
{
  [[nodiscard]] types::unique_instance create_instance( types::window const & window,
                                                        std::string const &   name ) noexcept
  {
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

    auto const validation_layers   = validation::validation_layers_data();
    auto const required_extensions = window.required_extensions();

    auto instance_create_info = [validation_layers, &required_extensions, &application_info]
    {
      auto info                    = VkInstanceCreateInfo();
      info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      info.pNext                   = validation::debug_create_info_ref();
      info.pApplicationInfo        = &application_info;
      info.enabledLayerCount       = static_cast<u32>( std::size( validation_layers ) );
      info.ppEnabledLayerNames     = std::data( validation_layers );
      info.enabledExtensionCount   = static_cast<u32>( std::size( required_extensions ) );
      info.ppEnabledExtensionNames = std::data( required_extensions );
      return info;
    }();

    return types::create_unique_instance( instance_create_info );
  }

  [[nodiscard]] types::unique_shader_module create_shader_module( types::device const              device,
                                                                  utility::slice<char const> const code ) noexcept
  {
    auto info     = VkShaderModuleCreateInfo();
    info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = static_cast<u32>( std::size( code ) );
    info.pCode    = reinterpret_cast<u32 const *>( std::data( code ) );
    return types::create_unique_shader_module( types::get( device ), info );
  }

  [[nodiscard]] types::unique_command_pool create_command_pool( types::device const            device,
                                                                types::queue_index const       queue_index,
                                                                VkCommandPoolCreateFlags const flags ) noexcept
  {
    auto info             = VkCommandPoolCreateInfo();
    info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.queueFamilyIndex = queue_index;
    info.flags            = flags;
    return types::create_unique_command_pool( types::get( device ), info );
  }

  [[nodiscard]] std::vector<types::unique_command_buffer> create_command_buffers( types::device const       device,
                                                                                  types::command_pool const pool,
                                                                                  u32                       count,
                                                                                  VkCommandBufferLevel level ) noexcept
  {
    auto info               = VkCommandBufferAllocateInfo();
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool        = types::get( pool );
    info.level              = level;
    info.commandBufferCount = count;
    return types::allocate_unique_command_buffers( types::get( device ), info );
  }

  [[nodiscard]] types::unique_device_memory create_device_memory( types::device const          device,
                                                                  types::physical_device const physical_device,
                                                                  types::buffer const          buffer,
                                                                  VkMemoryPropertyFlags const  properties ) noexcept
  {
    auto requirements = VkMemoryRequirements();
    vkGetBufferMemoryRequirements( types::get( device ), types::get( buffer ), &requirements );

    auto const memory_type_index =
      find_memory_type( types::get( physical_device ), requirements.memoryTypeBits, properties );

    MVK_VERIFY( memory_type_index.has_value() );

    auto allocate_info            = VkMemoryAllocateInfo();
    allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize  = requirements.size;
    allocate_info.memoryTypeIndex = memory_type_index.value();

    auto tmp = types::create_unique_device_memory( types::get( device ), allocate_info );
    vkBindBufferMemory( types::parent( tmp ), types::get( buffer ), types::get( tmp ), 0 );
    return tmp;
  }

  [[nodiscard]] types::unique_device_memory create_device_memory( types::device const          device,
                                                                  types::physical_device const physical_device,
                                                                  types::image const           buffer,
                                                                  VkMemoryPropertyFlags const  properties ) noexcept
  {
    auto requirements = VkMemoryRequirements();
    vkGetImageMemoryRequirements( types::get( device ), types::get( buffer ), &requirements );

    auto const memory_type_index =
      find_memory_type( types::get( physical_device ), requirements.memoryTypeBits, properties );

    MVK_VERIFY( memory_type_index.has_value() );

    auto allocate_info            = VkMemoryAllocateInfo();
    allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize  = requirements.size;
    allocate_info.memoryTypeIndex = memory_type_index.value();

    auto tmp = types::create_unique_device_memory( types::get( device ), allocate_info );
    vkBindImageMemory( types::parent( tmp ), types::get( buffer ), types::get( tmp ), 0 );
    return tmp;
  }

}  // namespace mvk::detail
