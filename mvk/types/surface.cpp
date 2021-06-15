#include "types/surface.hpp"
#include "utility/verify.hpp"

namespace mvk::types
{

surface::surface(VkInstance const instance,
                 GLFWwindow * const window) noexcept
    : wrapper(nullptr, instance)
{
  [[maybe_unused]] auto const result =
      glfwCreateWindowSurface(parent(), window, nullptr, &get());

  MVK_VERIFY(VK_SUCCESS == result);
}

[[nodiscard]] VkSurfaceCapabilitiesKHR
surface::query_capabilities(VkPhysicalDevice physical_device) const noexcept
{
  auto capabilities = VkSurfaceCapabilitiesKHR();
  [[maybe_unused]] auto const result =
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, get(),
                                                &capabilities);
  MVK_VERIFY(result == VK_SUCCESS);
  return capabilities;
}

} // namespace mvk::types
