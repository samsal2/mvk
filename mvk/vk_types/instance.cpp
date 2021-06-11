#include "vk_types/instance.hpp"

namespace mvk::vk_types
{

instance::instance(VkInstanceCreateInfo const & create_info) : unique_wrapper(nullptr)
{
    [[maybe_unused]] auto const result = vkCreateInstance(&create_info, nullptr, &reference());
    MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::vk_types
