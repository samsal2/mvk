#include "Engine/Misc.hpp"

#include "Detail/Misc.hpp"
#include "Detail/Readers.hpp"
#include "Engine/Renderer.hpp"

namespace Mvk::Engine {

void testRun(Renderer &Render) noexcept {
  auto Generator = Render.createDrawCallGenerator();

  auto Pvm = createTestPvm(Render);
  auto [Texture, Width, Height] =
      Detail::loadTex("../../assets/viking_room.png");
  auto [Vtx, Idx] = Detail::readObj("../../assets/viking_room.obj");
  Render.preDraw();
  auto Cmd = Generator->create(Vtx, Idx, Pvm, {Width, Height, Texture});
  Render.beginDraw();
  Render.draw(Cmd);
  Render.endDraw();

  while (!Render.notDone()) {
    glfwPollEvents();

    Render.preDraw();
    auto NewPvm = createTestPvm(Render);
    Generator->updatePvm(NewPvm, Cmd);
    Render.beginDraw();
    Render.draw(Cmd);
    Render.endDraw();

    Generator->nextBuffers();
  }

  Render.waitIdle();
}

[[nodiscard]] PVM createTestPvm(Renderer const &Render) noexcept {
  auto const time = Render.getCurrentTime();
  auto const SwapchainExtent = Render.getSwapchainExtent();

  constexpr auto turn_rate = glm::radians(90.0F);

  auto ubo = PVM();

  ubo.model = glm::rotate(glm::mat4(1.0F), time * turn_rate,
                          glm::vec3(0.0F, 0.0F, 1.0F));
  ubo.view =
      glm::lookAt(glm::vec3(2.0F, 2.0F, 2.0F), glm::vec3(0.0F, 0.0F, 0.0F),
                  glm::vec3(0.0F, 0.0F, 1.0F));

  auto const ratio = static_cast<float>(SwapchainExtent.width) /
                     static_cast<float>(SwapchainExtent.height);

  ubo.proj = glm::perspective(glm::radians(45.0F), ratio, 0.1F, 10.0F);
  ubo.proj[1][1] *= -1;

  return ubo;
}

} // namespace Mvk::Engine
