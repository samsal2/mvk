#include "types/instance.hpp"

namespace mvk::types
{

instance::instance(VkInstanceCreateInfo const & create_info) : wrapper()
{
    [[maybe_unused]] auto const result = vkCreateInstance(&create_info, nullptr, &reference());
    MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::types
