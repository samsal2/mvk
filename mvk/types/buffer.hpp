#ifndef MVK_TYPES_BUFFER_HPP_INCLUDED
#define MVK_TYPES_BUFFER_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"
#include "utility/slice.hpp"
#include "utility/types.hpp"

namespace mvk::types
{

class device;
class command_pool;

class buffer : public detail::wrapper<detail::deleter<vkDestroyBuffer>,
                                      detail::handle<VkBuffer>,
                                      detail::parent<VkDevice>>
{
public:
  constexpr buffer() noexcept = default;

  buffer(VkDevice device, VkBufferCreateInfo const & info) noexcept;

  [[nodiscard]] constexpr device_size
  size() const noexcept;

  [[nodiscard]] constexpr VkMemoryRequirements
  memory_requirements() const noexcept;

  // TODO(samuel): At the moment the staging calls create a new buffer and
  // it's memory when called, not good

  // When calling buffer must've had been bound before hand
  buffer &
  stage(device const & device, command_pool const & command_pool,
        utility::slice<std::byte> src, device_size offset = 0) noexcept;

private:
  VkMemoryRequirements memory_requirements_ = {};
};

[[nodiscard]] constexpr device_size
buffer::size() const noexcept
{
  return memory_requirements_.size;
}

[[nodiscard]] constexpr VkMemoryRequirements
buffer::memory_requirements() const noexcept
{
  return memory_requirements_;
}

} // namespace mvk::types

#endif
