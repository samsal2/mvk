#include "types/descriptor_pool.hpp"

namespace mvk::types
{

descriptor_pool::descriptor_pool(
    VkDevice const device, VkDescriptorPoolCreateInfo const & info) noexcept
    : wrapper(nullptr, device)
{
  [[maybe_unused]] auto const result =
      vkCreateDescriptorPool(parent(), &info, nullptr, &get());

  MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::types
