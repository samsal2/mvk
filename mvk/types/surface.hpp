#ifndef MVK_TYPES_SURFACE_HPP_INCLUDED
#define MVK_TYPES_SURFACE_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/checkers.hpp"
#include "types/detail/misc.hpp"
#include "types/detail/wrapper.hpp"

namespace mvk::types
{

class surface : public detail::wrapper<detail::deleter<vkDestroySurfaceKHR>,
                                       detail::handle<VkSurfaceKHR>,
                                       detail::parent<VkInstance>>
{
public:
  constexpr surface() noexcept = default;

  surface(VkInstance instance, GLFWwindow * window) noexcept;

  template <typename Checker = decltype(detail::default_format_checker)>
  requires detail::requirement_checker<Checker>
  [[nodiscard]] VkSurfaceFormatKHR
  query_format(
      VkPhysicalDevice physical_device,
      Checker && check = detail::default_format_checker) const noexcept;

  [[nodiscard]] VkSurfaceCapabilitiesKHR
  query_capabilities(VkPhysicalDevice physical_device) const noexcept;
};

template <typename Checker>
requires detail::requirement_checker<Checker>
[[nodiscard]] VkSurfaceFormatKHR
surface::query_format(VkPhysicalDevice const physical_device,
                      Checker && check) const noexcept
{
  return detail::choose_surface_format(physical_device, get(),
                                       std::forward<Checker>(check));
}

} // namespace mvk::types

#endif
