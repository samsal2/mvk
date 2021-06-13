#include "types/semaphore.hpp"

namespace mvk::types
{

semaphore::semaphore(VkDevice const device,
                     VkSemaphoreCreateInfo const & info)
    : wrapper(nullptr, device)
{
  [[maybe_unused]] auto const result =
      vkCreateSemaphore(parent(), &info, nullptr, &get());
  MVK_VERIFY(VK_SUCCESS == result);
}
} // namespace mvk::types
