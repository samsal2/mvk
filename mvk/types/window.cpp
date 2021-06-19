#include "types/window.hpp"

#include "types/validation/validation.hpp"
#include "utility/slice.hpp"

namespace mvk::types
{

window::window(extent const extent) noexcept
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  auto const callback = [](GLFWwindow * const window_ptr,
                           [[maybe_unused]] int const callback_width,
                           [[maybe_unused]] int const callback_height)
  {
    auto const user_ptr = glfwGetWindowUserPointer(window_ptr);
    auto const ctx = reinterpret_cast<window *>(user_ptr);
    ctx->set_framebuffer_resized(true);
  };

  auto const [width, height] = extent;

  auto ptr = glfwCreateWindow(width, height, "stan loona", nullptr, nullptr);
  instance_ = std::unique_ptr<GLFWwindow, deleter>(ptr);

  glfwSetWindowUserPointer(get(), this);
  glfwSetFramebufferSizeCallback(get(), callback);
}

void
window::deleter::operator()(GLFWwindow * const window) const noexcept
{
  if (window != nullptr) [[likely]]
  {
    glfwDestroyWindow(window);
    glfwTerminate();
  }
}

void
window::set_framebuffer_resized(bool const resized) noexcept
{
  framebuffer_resized_ = resized;
}

[[nodiscard]] std::vector<char const *>
window::required_extensions() const noexcept
{

  auto const glfw_extensions = []
  {
    auto count = uint32_t(0);
    auto const data = glfwGetRequiredInstanceExtensions(&count);
    return utility::slice(data, count);
  }();

  auto instance_extensions = validation::required_instance_extensions();

  auto extensions = std::vector<char const *>();

  extensions.insert(std::end(extensions), std::begin(instance_extensions),
                    std::end(instance_extensions));

  extensions.insert(std::end(extensions), std::begin(glfw_extensions),
                    std::end(glfw_extensions));

  return extensions;
}

[[nodiscard]] window::extent
window::query_framebuffer_size() const noexcept
{

  auto width = 0;
  auto height = 0;

  do
  {
    glfwGetFramebufferSize(get(), &width, &height);
    glfwWaitEvents();
  } while (width == 0 || height == 0);

  return {width, height};
}

[[nodiscard]] bool
window::should_close() const noexcept
{
  return glfwWindowShouldClose(get()) != 0;
}

} // namespace mvk::types
