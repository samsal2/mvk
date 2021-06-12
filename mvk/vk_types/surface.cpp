#include "vk_types/surface.hpp"

namespace mvk::vk_types
{

surface::surface(VkInstance const instance, GLFWwindow * const window) : wrapper(nullptr, instance)
{
    [[maybe_unused]] auto const result = glfwCreateWindowSurface(parent(), window, nullptr, &reference());

    MVK_VERIFY(VK_SUCCESS == result);
}

[[nodiscard]] VkSurfaceCapabilitiesKHR
surface::query_capabilities(VkPhysicalDevice physical_device) const noexcept
{
    auto capabilities = VkSurfaceCapabilitiesKHR();
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, get(), &capabilities);
    return capabilities;
}

} // namespace mvk::vk_types
