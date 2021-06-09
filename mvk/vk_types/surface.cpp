#include "vk_types/surface.hpp"

namespace mvk::vk_types
{

surface::surface(VkInstance const instance, GLFWwindow * const window)
  : wrapper(nullptr, make_deleter(instance))
{
  [[maybe_unused]] auto const result =
    glfwCreateWindowSurface(parent(), window, nullptr, &reference());

  MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::vk_types
