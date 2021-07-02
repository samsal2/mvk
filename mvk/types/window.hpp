#ifndef MVK_TYPES_WINDOW_HPP_INCLUDED
#define MVK_TYPES_WINDOW_HPP_INCLUDED

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <memory>

namespace mvk::types
{
  class window
  {
  public:
    struct extent
    {
      int width_;
      int height_;
    };

    constexpr window() noexcept = default;

    explicit window( extent extent ) noexcept;

    [[nodiscard]] constexpr GLFWwindow * get() const noexcept;

    void set_framebuffer_resized( bool resized ) noexcept;

    [[nodiscard]] constexpr bool framebuffer_resized() const noexcept;

    [[nodiscard]] std::vector<char const *> required_extensions() const noexcept;

    [[nodiscard]] extent query_framebuffer_size() const noexcept;

    [[nodiscard]] bool should_close() const noexcept;

  private:
    struct deleter
    {
      void operator()( GLFWwindow * window ) const noexcept;
    };

    std::unique_ptr<GLFWwindow, deleter> instance_;
    bool                                 framebuffer_resized_ = false;
  };

  [[nodiscard]] constexpr GLFWwindow * window::get() const noexcept
  {
    return instance_.get();
  }

  [[nodiscard]] constexpr bool window::framebuffer_resized() const noexcept
  {
    return framebuffer_resized_;
  }

}  // namespace mvk::types

#endif
