#ifndef MVK_VK_TYPES_GLFW_CONTEXT_HPP_INCLUDED
#define MVK_VK_TYPES_GLFW_CONTEXT_HPP_INCLUDED

#include "vk_types/common.hpp"

namespace mvk::vk_types
{

class window
{
public:
  constexpr window() noexcept = default;
  window(int width, int height);

  [[nodiscard]] constexpr GLFWwindow *
  get() const noexcept;

  void
  set_framebuffer_resized(bool resized) noexcept;

  [[nodiscard]] constexpr bool
  framebuffer_resized() const noexcept;

  [[nodiscard]] std::vector<char const *>
  required_extensions() const noexcept;

private:
  struct deleter
  {
    void
    operator()(GLFWwindow * window) const noexcept;
  };

  std::unique_ptr<GLFWwindow, deleter> instance_;
  bool                                 framebuffer_resized_ = false;
};

[[nodiscard]] constexpr GLFWwindow *
window::get() const noexcept
{
  return instance_.get();
}

[[nodiscard]] constexpr bool
window::framebuffer_resized() const noexcept
{
  return framebuffer_resized_;
}

} // namespace mvk::vk_types

#endif
