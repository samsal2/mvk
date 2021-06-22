#include "detail/creators.hpp"

namespace mvk::detail
{
[[nodiscard]] types::unique_instance
create_instance(types::window const & window,
                std::string const & name) noexcept
{
  auto application_info = [&name]
  {
    auto info = VkApplicationInfo();
    info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    info.pApplicationName = name.c_str();
    info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    info.pEngineName = "No Engine";
    info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    return info;
  }();

  auto const validation_layers = validation::validation_layers_data();
  auto const required_extensions = window.required_extensions();

  auto instance_create_info =
      [validation_layers, &required_extensions, &application_info]
  {
    auto const [validation_data, validation_count] =
        utility::bind_data_and_size(validation_layers);
    auto const [required_data, required_count] =
        utility::bind_data_and_size(required_extensions);

    auto info = VkInstanceCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pNext = validation::debug_create_info_ref();
    info.pApplicationInfo = &application_info;
    info.enabledLayerCount = static_cast<uint32_t>(validation_count);
    info.ppEnabledLayerNames = validation_data;
    info.enabledExtensionCount = static_cast<uint32_t>(required_count);
    info.ppEnabledExtensionNames = required_data;
    return info;
  }();

  // TODO(samuel): missing creators
  auto handle = VkInstance();
  vkCreateInstance(&instance_create_info, nullptr, &handle);

  return types::unique_instance(handle);
}

[[nodiscard]] types::unique_shader_module
create_shader_module(types::device const device,
                     utility::slice<char> const code) noexcept
{
  auto info = VkShaderModuleCreateInfo();
  info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.codeSize = static_cast<uint32_t>(std::size(code));
  info.pCode = reinterpret_cast<uint32_t const *>(std::data(code));
  return types::unique_shader_module::create(types::get(device), info);
}

[[nodiscard]] types::unique_command_pool
create_command_pool(types::device const device,
                    types::queue_index const queue_index,
                    VkCommandPoolCreateFlags const flags) noexcept
{
  auto info = VkCommandPoolCreateInfo();
  info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  info.queueFamilyIndex = queue_index;
  info.flags = flags;
  return types::unique_command_pool::create(types::get(device), info);
}

[[nodiscard]] std::vector<types::unique_command_buffer>
create_command_buffers(types::device const device,
                       types::command_pool const pool, uint32_t count,
                       VkCommandBufferLevel level) noexcept
{
  auto info = VkCommandBufferAllocateInfo();
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  info.commandPool = types::get(pool);
  info.level = level;
  info.commandBufferCount = count;
  return types::unique_command_buffer::allocate(types::get(device), info);
}

[[nodiscard]] types::unique_device_memory
create_device_memory(types::device const device,
                     types::physical_device const physical_device,
                     types::buffer const buffer,
                     VkMemoryPropertyFlags const properties) noexcept
{
  auto const requirements = query<vkGetBufferMemoryRequirements>::with(
      types::get(device), types::get(buffer));

  auto const memory_type_index = find_memory_type(
      types::get(physical_device), requirements.memoryTypeBits, properties);

  MVK_VERIFY(memory_type_index.has_value());

  auto allocate_info = VkMemoryAllocateInfo();
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.allocationSize = requirements.size;
  allocate_info.memoryTypeIndex = memory_type_index.value();

  auto tmp =
      types::unique_device_memory::create(types::get(device), allocate_info);
  vkBindBufferMemory(types::parent(tmp), types::get(buffer), types::get(tmp),
                     0);
  return tmp;
}

[[nodiscard]] types::unique_device_memory
create_device_memory(types::device const device,
                     types::physical_device const physical_device,
                     types::image const buffer,
                     VkMemoryPropertyFlags const properties) noexcept
{
  auto const requirements = query<vkGetImageMemoryRequirements>::with(
      types::get(device), types::get(buffer));

  auto const memory_type_index = find_memory_type(
      types::get(physical_device), requirements.memoryTypeBits, properties);

  MVK_VERIFY(memory_type_index.has_value());

  auto allocate_info = VkMemoryAllocateInfo();
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.allocationSize = requirements.size;
  allocate_info.memoryTypeIndex = memory_type_index.value();

  auto tmp =
      types::unique_device_memory::create(types::get(device), allocate_info);
  vkBindImageMemory(types::parent(tmp), types::get(buffer), types::get(tmp),
                    0);
  return tmp;
}

} // namespace mvk::detail
