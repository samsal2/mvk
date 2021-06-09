#ifndef MVK_VK_TYPES_BUFFER_HPP_INCLUDED
#define MVK_VK_TYPES_BUFFER_HPP_INCLUDED

#include "utility/slice.hpp"
#include "utility/types.hpp"
#include "vk_types/common.hpp"
#include "vk_types/detail/wrapper.hpp"

namespace mvk::vk_types
{

class device;
class command_pool;

class buffer : public detail::wrapper<VkBuffer, vkDestroyBuffer>
{
public:
  constexpr buffer() noexcept = default;
  buffer(VkDevice device, VkBufferCreateInfo const & create_info);

  [[nodiscard]] constexpr VkDeviceSize
  size() const noexcept;

  [[nodiscard]] constexpr VkMemoryRequirements
  memory_requirements() const noexcept;

  // TODO(samuel): At the moment the staging calls create a new buffer and
  // it's memory when called, not good

  // When calling buffer must've had been bound before hand
  buffer &
  stage(
    device const &            device,
    command_pool const &      command_pool,
    utility::slice<std::byte> data_source,
    VkDeviceSize              offset = 0);

private:
  VkMemoryRequirements memory_requirements_ = {};
};

[[nodiscard]] constexpr VkDeviceSize
buffer::size() const noexcept
{
  return memory_requirements_.size;
}

[[nodiscard]] constexpr VkMemoryRequirements
buffer::memory_requirements() const noexcept
{
  return memory_requirements_;
}

} // namespace mvk::vk_types

#endif
