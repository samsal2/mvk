#ifndef MVK_VK_TYPES_IMAGE_VIEW_HPP_INCLUDED
#define MVK_VK_TYPES_IMAGE_VIEW_HPP_INCLUDED

#include "vk_types/common.hpp"
#include "vk_types/detail/wrapper.hpp"

namespace mvk::vk_types
{

class image_view : public detail::wrapper<detail::deleter<vkDestroyImageView>, detail::handle<VkImageView>, detail::parent<VkDevice>>
{
public:
    constexpr image_view() noexcept = default;
    image_view(VkDevice device, VkImageViewCreateInfo const & create_info);
};

} // namespace mvk::vk_types

#endif
