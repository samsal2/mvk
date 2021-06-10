#include "vk_types/device_memory.hpp"

#include "vk_types/buffer.hpp"
#include "vk_types/device.hpp"
#include "vk_types/image.hpp"

namespace mvk::vk_types
{

device_memory::device_memory(
  VkDevice const               device,
  VkMemoryAllocateInfo const & allocate_info)
  : wrapper(nullptr, make_deleter(device))
{
  [[maybe_unused]] auto const result =
    vkAllocateMemory(parent(), &allocate_info, nullptr, &reference());

  MVK_VERIFY(VK_SUCCESS == result);
}

device_memory::device_memory(device_memory && other) noexcept
  : wrapper(std::move(other))
{
  std::swap(data_, other.data_);
}

device_memory &
device_memory::operator=(device_memory && other) noexcept
{
  std::swap(data_, other.data_);
  wrapper::operator=(std::move(other));
  return *this;
}

device_memory::~device_memory() noexcept
{
  if (data_ != nullptr)
  {
    unmap();
  }
}

device_memory &
device_memory::bind(buffer const & buffer, VkDeviceSize const offset)
{
  [[maybe_unused]] auto const result =
    vkBindBufferMemory(parent(), buffer.get(), get(), offset);

  MVK_VERIFY(VK_SUCCESS == result);
  return *this;
}

device_memory &
device_memory::bind(image const & image, VkDeviceSize const offset)
{
  [[maybe_unused]] auto const result =
    vkBindImageMemory(parent(), image.get(), get(), offset);

  MVK_VERIFY(VK_SUCCESS == result);
  return *this;
}

device_memory &
device_memory::map(VkDeviceSize const size, VkDeviceSize const offset)
{
  [[maybe_unused]] auto const result =
    vkMapMemory(parent(), get(), offset, size, 0, &data_);

  MVK_VERIFY(VK_SUCCESS == result);
  return *this;
}

device_memory &
device_memory::unmap() noexcept
{
  vkUnmapMemory(parent(), get());
  data_ = nullptr;

  return *this;
}

device_memory &
device_memory::copy_data(
  utility::slice<std::byte> data_source,
  VkDeviceSize const        offset)
{
  MVK_VERIFY(data_);
  auto const [data, size] = utility::bind_data_and_size(data_source);
  auto const destination  = static_cast<std::byte *>(data_) + offset;
  std::memcpy(destination, data, size);
  return *this;
}

} // namespace mvk::vk_types
