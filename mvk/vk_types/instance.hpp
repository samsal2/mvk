#ifndef MVK_VK_TYPES_INSTANCE_HPP_INCLUDED
#define MVK_VK_TYPES_INSTANCE_HPP_INCLUDED

#include "vk_types/common.hpp"
#include "vk_types/detail/wrapper.hpp"

namespace mvk::vk_types
{

class instance : public detail::wrapper<VkInstance, vkDestroyInstance>
{
public:
        constexpr instance() noexcept = default;
        explicit instance(VkInstanceCreateInfo const & create_info);
};

} // namespace mvk::vk_types

#endif // MVK_VK_TYPES_INSTANCE_HPP_INCLUDED
