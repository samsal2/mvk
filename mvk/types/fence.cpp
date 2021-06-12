#include "types/fence.hpp"

namespace mvk::types
{

fence::fence(VkDevice const device, VkFenceCreateInfo const & create_info)
    : wrapper(nullptr, device)
{
  [[maybe_unused]] auto const result =
      vkCreateFence(parent(), &create_info, nullptr, &reference());
  MVK_VERIFY(VK_SUCCESS == result);
}

fence &
fence::reset()
{
  [[maybe_unused]] auto const result =
      vkResetFences(parent(), 1, &reference());
  MVK_VERIFY(VK_SUCCESS == result);
  return *this;
}

fence &
fence::wait()
{
  [[maybe_unused]] auto const result =
      vkWaitForFences(parent(), 1, &reference(), VK_TRUE,
                      std::numeric_limits<int64_t>::max());
  MVK_VERIFY(VK_SUCCESS == result);
  return *this;
}

} // namespace mvk::types
