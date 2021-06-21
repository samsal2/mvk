#ifndef MVK_TYPES_DETAIL_CHECKERS_HPP_INCLUDED
#define MVK_TYPES_DETAIL_CHECKERS_HPP_INCLUDED

#include "utility/concepts.hpp"

#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

namespace mvk::detail
{

template <typename Callable>
concept requirement_checker = requires
{
  requires utility::callable<Callable, bool, VkSurfaceFormatKHR const &>;
};

template <typename Callable>
concept result_checker = requires
{
  requires utility::callable<Callable, void, VkResult>;
};

void
default_result_checker(VkResult result);

[[nodiscard]] static constexpr bool
default_format_checker(VkSurfaceFormatKHR const & format) noexcept;

} // namespace mvk::detail

namespace mvk::detail
{

[[nodiscard]] static constexpr bool
default_format_checker(VkSurfaceFormatKHR const & format) noexcept
{
  return (format.format == VK_FORMAT_B8G8R8A8_SRGB) &&
         (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
}

} // namespace mvk::detail
#endif
