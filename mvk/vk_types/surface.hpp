#ifndef MVK_VK_TYPES_SURFACE_HPP_INCLUDED
#define MVK_VK_TYPES_SURFACE_HPP_INCLUDED

#include "vk_types/common.hpp"
#include "vk_types/detail/checkers.hpp"
#include "vk_types/detail/misc.hpp"
#include "vk_types/detail/wrapper.hpp"

namespace mvk::vk_types
{

class surface : public detail::wrapper<VkSurfaceKHR, vkDestroySurfaceKHR>
{
public:
  constexpr surface() noexcept = default;
  surface(VkInstance instance, GLFWwindow * window);

  template <typename Checker = decltype(detail::default_format_checker)>
  requires detail::requirement_checker<Checker>
  [[nodiscard]] VkSurfaceFormatKHR
  find_format(
    VkPhysicalDevice physical_device,
    Checker &&       check = detail::default_format_checker) const noexcept;
};

template <typename Checker>
requires detail::requirement_checker<Checker>
[[nodiscard]] VkSurfaceFormatKHR
surface::find_format(VkPhysicalDevice const physical_device, Checker && check)
  const noexcept
{
  return detail::choose_surface_format(
    physical_device,
    get(),
    std::forward<Checker>(check));
}

} // namespace mvk::vk_types

#endif
