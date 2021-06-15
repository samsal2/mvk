#ifndef MVK_TYPES_IMAGE_VIEW_HPP_INCLUDED
#define MVK_TYPES_IMAGE_VIEW_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"

namespace mvk::types
{

class image_view : public detail::wrapper<detail::deleter<vkDestroyImageView>,
                                          detail::handle<VkImageView>,
                                          detail::parent<VkDevice>>
{
public:
  constexpr image_view() noexcept = default;

  image_view(VkDevice device, VkImageViewCreateInfo const & info) noexcept;
};

} // namespace mvk::types

#endif
