#ifndef MVK_FACTORIES_HPP_INCLUDED
#define MVK_FACTORIES_HPP_INCLUDED

#include "utility/slice.hpp"
#include "utility/verify.hpp"
#include "vk_types/vk_types.hpp"

namespace mvk::factories
{

namespace detail
{

[[nodiscard]] bool
is_extension_present(
  std::string const &                   extension_name,
  utility::slice<VkExtensionProperties> extensions) noexcept;

[[nodiscard]] bool
check_extension_support(
  VkPhysicalDevice             physical_device,
  utility::slice<char const *> device_extensions) noexcept;

[[nodiscard]] VkPhysicalDevice
choose_physical_device(
  VkInstance                   instance,
  VkSurfaceKHR                 surface,
  utility::slice<char const *> device_extensions) noexcept;

[[nodiscard]] constexpr bool
meets_graphic_requirements(
  VkQueueFamilyProperties const & queue_family) noexcept;

[[nodiscard]] bool
check_format_and_present_mode_availability(
  VkPhysicalDevice physical_device,
  VkSurfaceKHR     surface) noexcept;

[[nodiscard]] bool
supports_surface(
  VkPhysicalDevice physical_device,
  VkSurfaceKHR     surface,
  uint32_t         index);

[[nodiscard]] std::optional<std::pair<uint32_t, uint32_t>>
query_family_indices(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

} // namespace detail

[[nodiscard]] vk_types::buffer
create_buffer(
  vk_types::device const & device,
  VkDeviceSize             size,
  VkBufferUsageFlags       usage,
  VkSharingMode            sharing_mode);

[[nodiscard]] vk_types::command_buffers
create_command_buffers(
  vk_types::command_pool const & command_pool,
  VkDeviceSize                   size);

[[nodiscard]] vk_types::command_pool
create_command_pool(vk_types::device const & device);

[[nodiscard]] vk_types::descriptor_pool
create_descriptor_pool(vk_types::device const & device, size_t max_sets);

[[nodiscard]] vk_types::descriptor_set_layout
create_descriptor_set_layout(vk_types::device const & device);

[[nodiscard]] vk_types::descriptor_sets
create_descriptor_sets(
  vk_types::descriptor_pool const &       descriptor_pool,
  vk_types::descriptor_set_layout const & descriptor_set_layout,
  vk_types::swapchain const &             swapchain);

template <typename T>
[[nodiscard]] vk_types::device_memory
create_device_memory(
  vk_types::device const & device,
  T const &                buffer,
  VkMemoryPropertyFlags    properties);

[[nodiscard]] vk_types::device
create_device(
  vk_types::surface const &    surface,
  utility::slice<char const *> device_extensions) noexcept;

[[nodiscard]] vk_types::fence
create_fence(vk_types::device const & device);

[[nodiscard]] vk_types::framebuffer
create_framebuffer(
  vk_types::swapchain const &   swapchain,
  vk_types::render_pass const & render_pass,
  vk_types::image_view const &  current_image_view,
  vk_types::image_view const &  depth_image_view);

[[nodiscard]] vk_types::image_view
create_image_view(
  vk_types::device const &  device,
  vk_types::surface const & surface,
  VkImage                   image);

[[nodiscard]] vk_types::image_view
create_image_view(
  vk_types::image const & image,
  VkFormat                format,
  VkImageAspectFlags      aspect);

[[nodiscard]] vk_types::image
create_image(
  vk_types::device const & device,
  uint32_t                 width,
  uint32_t                 height,
  VkFormat                 format,
  VkImageTiling            tiling,
  VkImageUsageFlags        usage,
  VkSharingMode            sharing_mode);

[[nodiscard]] vk_types::image
create_image(
  vk_types::device const &         device,
  vk_types::image::texture const & texture);

[[nodiscard]] vk_types::instance
create_instance(
  std::string const &          name,
  utility::slice<char const *> required_extensions);

[[nodiscard]] vk_types::pipeline_layout
create_pipeline_layout(
  vk_types::descriptor_set_layout const & descriptor_set_layout);

[[nodiscard]] vk_types::pipeline
create_pipeline(
  vk_types::swapchain const &                     swapchain,
  vk_types::render_pass const &                   render_pass,
  vk_types::pipeline_layout const &               layout,
  utility::slice<VkPipelineShaderStageCreateInfo> shader_stages);

[[nodiscard]] vk_types::render_pass
create_render_pass(
  vk_types::device const &  device,
  vk_types::surface const & surface);

[[nodiscard]] vk_types::sampler
create_sampler(vk_types::device const & device, uint32_t mipmap_levels);

[[nodiscard]] vk_types::semaphore
create_semaphore(vk_types::device const & device);

[[nodiscard]] vk_types::shader_module
create_shader_module(vk_types::device const & device, std::string_view code);

[[nodiscard]] vk_types::surface
create_surface(vk_types::instance const & instance, GLFWwindow * window);

[[nodiscard]] vk_types::swapchain
create_swapchain(
  vk_types::device const &  device,
  vk_types::surface const & surface,
  VkExtent2D                extent);

} // namespace mvk::factories

namespace mvk::factories
{

template <typename T>
[[nodiscard]] vk_types::device_memory
create_device_memory(
  vk_types::device const &    device,
  T const &                   buffer,
  VkMemoryPropertyFlags const properties)
{
  auto const requirements = buffer.memory_requirements();

  using vk_types::detail::find_memory_type;
  auto const memory_type_index = find_memory_type(
    device.physical_device(),
    requirements.memoryTypeBits,
    properties);

  MVK_VERIFY(memory_type_index.has_value());

  auto allocate_info            = VkMemoryAllocateInfo();
  allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.allocationSize  = requirements.size;
  allocate_info.memoryTypeIndex = memory_type_index.value();

  auto device_memory_tmp =
    vk_types::device_memory(device.get(), allocate_info);
  device_memory_tmp.bind(buffer);

  return device_memory_tmp;
}

} // namespace mvk::factories

#endif // MVK_FACTORIES_HPP_INCLUDED
