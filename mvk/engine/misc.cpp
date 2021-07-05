#include "engine/misc.hpp"

#include "engine/draw.hpp"

namespace mvk::engine
{
  void testRun(InOut<Context> Ctx) noexcept
  {
    while (glfwWindowShouldClose(Ctx->Window) == 0)
    {
      glfwPollEvents();

      beginDraw(Ctx);

      auto const pvm = createTestPvm(Ctx);

      basicDraw(Ctx, utility::as_bytes(Ctx->vertices_), utility::as_bytes(Ctx->indices_), utility::as_bytes(pvm));

      endDraw(Ctx);
    }

    vkDeviceWaitIdle(Ctx->Device);
  }

  [[nodiscard]] pvm createTestPvm(In<Context> Ctx) noexcept
  {
    auto const time = getCurrentTime(Ctx);

    constexpr auto turn_rate = glm::radians(90.0F);

    auto ubo = pvm();

    ubo.model = glm::rotate(glm::mat4(1.0F), time * turn_rate, glm::vec3(0.0F, 0.0F, 1.0F));
    ubo.view  = glm::lookAt(glm::vec3(2.0F, 2.0F, 2.0F), glm::vec3(0.0F, 0.0F, 0.0F), glm::vec3(0.0F, 0.0F, 1.0F));

    auto const ratio = static_cast<float>(Ctx->SwapchainExtent.width) / static_cast<float>(Ctx->SwapchainExtent.height);

    ubo.proj = glm::perspective(glm::radians(45.0F), ratio, 0.1F, 10.0F);
    ubo.proj[1][1] *= -1;

    return ubo;
  }

}  // namespace mvk::engine
