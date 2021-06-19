#include "detail/checkers.hpp"

namespace mvk::detail
{

void
default_result_checker([[maybe_unused]] VkResult const result)
{
  MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::detail
