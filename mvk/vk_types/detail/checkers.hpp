#ifndef MVK_VK_TYPES_DETAIL_CHECKERS_HPP_INCLUDED
#define MVK_VK_TYPES_DETAIL_CHECKERS_HPP_INCLUDED

#include "utility/concepts.hpp"
#include "vk_types/common.hpp"

#include <utility>
#include <vector>

namespace mvk::vk_types::detail
{

template <typename Callable>
concept requirement_checker = requires
{
        {utility::callable<Callable, bool, VkSurfaceFormatKHR const &>};
};

template <typename Callable>
concept result_checker = requires
{
        {utility::callable<Callable, void, VkResult>};
};

void
default_result_checker(VkResult result);

[[nodiscard]] static constexpr bool
default_format_checker(VkSurfaceFormatKHR const & format) noexcept;

} // namespace mvk::vk_types::detail

namespace mvk::vk_types::detail
{

[[nodiscard]] static constexpr bool
default_format_checker(VkSurfaceFormatKHR const & format) noexcept
{
        return (format.format == VK_FORMAT_B8G8R8A8_SRGB) && (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
}

} // namespace mvk::vk_types::detail
#endif
