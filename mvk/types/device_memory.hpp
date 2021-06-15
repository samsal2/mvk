#ifndef MVK_TYPES_DEVICE_MEMORY_HPP_INCLUDED
#define MVK_TYPES_DEVICE_MEMORY_HPP_INCLUDED

#include "types/buffer.hpp"
#include "types/common.hpp"
#include "types/detail/wrapper.hpp"
#include "utility/slice.hpp"
#include "utility/types.hpp"

namespace mvk::types
{

class image;

class device_memory : public detail::wrapper<detail::deleter<vkFreeMemory>,
                                             detail::handle<VkDeviceMemory>,
                                             detail::parent<VkDevice>>
{
public:
  constexpr device_memory() noexcept = default;

  device_memory(VkDevice device, VkMemoryAllocateInfo const & info) noexcept;

  device_memory(device_memory const &) = delete;
  device_memory(device_memory && other) noexcept;

  device_memory &
  operator=(device_memory const &) noexcept = delete;

  device_memory &
  operator=(device_memory && other) noexcept;

  ~device_memory() noexcept;

  device_memory &
  bind(buffer const & buffer, device_size offset = 0) noexcept;

  device_memory &
  bind(image const & image, device_size offset = 0) noexcept;

  // NOTE: the destructor also takes care of unmapping, so unless there is
  //       a requirement to map another size there is no need to call this
  device_memory &
  unmap() noexcept;

  device_memory &
  map(device_size size = VK_WHOLE_SIZE, device_size offset = 0) noexcept;

  device_memory &
  copy_data(utility::slice<std::byte> src, device_size offset = 0) noexcept;

private:
  void * data_ = nullptr;
};

} // namespace mvk::types

#endif
