#ifndef MVK_VK_TYPES_DEVICE_MEMORY_HPP_INCLUDED
#define MVK_VK_TYPES_DEVICE_MEMORY_HPP_INCLUDED

#include "utility/slice.hpp"
#include "utility/types.hpp"
#include "vk_types/buffer.hpp"
#include "vk_types/common.hpp"
#include "vk_types/detail/wrapper.hpp"

namespace mvk::vk_types
{

class image;

class device_memory : public detail::wrapper<VkDeviceMemory, vkFreeMemory>
{
public:
  constexpr device_memory() noexcept = default;
  device_memory(VkDevice device, VkMemoryAllocateInfo const & allocate_info);

  device_memory(device_memory const &) = delete;
  device_memory(device_memory && other) noexcept;

  device_memory &
  operator=(device_memory const &) noexcept = delete;

  device_memory &
  operator=(device_memory && other) noexcept;

  ~device_memory() noexcept;

  device_memory &
  bind(buffer const & buffer, VkDeviceSize offset = 0);

  device_memory &
  bind(image const & image, VkDeviceSize offset = 0);

  // NOTE: the destructor also takes care of unmapping, so unless there is
  //       a requirement to map another size there is no need to call this
  device_memory &
  unmap() noexcept;

  device_memory &
  map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

  device_memory &
  copy_data(utility::slice<std::byte> data_source, VkDeviceSize offset = 0);

private:
  void * data_ = nullptr;
};

} // namespace mvk::vk_types

#endif