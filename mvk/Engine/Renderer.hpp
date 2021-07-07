#pragma once

#include "Engine/Context.hpp"
#include "Engine/DrawCallGenerator.hpp"

#include <string>

namespace Mvk::Engine {

class Renderer {
public:
  Renderer(std::string const &Name, VkExtent2D Extent) noexcept;

  [[nodiscard]] std::unique_ptr<DrawCallGenerator>
  createDrawCallGenerator() noexcept;

  [[nodiscard]] bool notDone() const noexcept {
    return glfwWindowShouldClose(Ctx->getWindow()) != 0;
  }
  void waitIdle() const noexcept { vkDeviceWaitIdle(Ctx->getDevice()); }

  [[nodiscard]] Context::Seconds getCurrentTime() const noexcept {
    return Ctx->getCurrentTime();
  }
  [[nodiscard]] VkExtent2D getSwapchainExtent() const noexcept {
    return Ctx->getSwapchainExtent();
  }

  void preDraw() noexcept;
  void beginDraw() noexcept;

  void draw(DrawCmd const &Cmd) noexcept;

  void endDraw() noexcept;

private:
  std::unique_ptr<Context> Ctx;
};

} // namespace Mvk::Engine

