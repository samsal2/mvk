#include "types/instance.hpp"

namespace mvk::types
{

instance::instance(VkInstanceCreateInfo const & info) : wrapper()
{
  [[maybe_unused]] auto const result =
      vkCreateInstance(&info, nullptr, &reference());
  MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::types
