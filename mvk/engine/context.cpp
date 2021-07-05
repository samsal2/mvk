#include "engine/context.hpp"

namespace mvk::engine
{
  [[nodiscard]] Seconds getCurrentTime(In<Context> Ctx) noexcept
  {
    auto const CurrentTime = std::chrono::high_resolution_clock::now();
    auto const DeltaTime   = CurrentTime - Ctx->StartTime;
    return std::chrono::duration<float, std::chrono::seconds::period>(DeltaTime).count();
  }

  void getFramebufferSize(In<Context> Ctx, Out<VkExtent2D> size) noexcept
  {
    *size = VkExtent2D();

    auto Width  = 0;
    auto Height = 0;

    do
    {
      glfwGetFramebufferSize(Ctx->Window, &Width, &Height);
      glfwWaitEvents();
    } while (Width == 0 || Height == 0);

    size->width  = static_cast<uint32_t>(Width);
    size->height = static_cast<uint32_t>(Height);
  }

}  // namespace mvk::engine
