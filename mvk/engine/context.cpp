#include "engine/Context.hpp"

namespace mvk::engine
{
  [[nodiscard]] float currentTime( Context const & Ctx ) noexcept
  {
    auto const CurrentTime = std::chrono::high_resolution_clock::now();
    auto const DeltaTime   = CurrentTime - Ctx.StartTime;
    return std::chrono::duration< float, std::chrono::seconds::period >( DeltaTime ).count();
  }

  [[nodiscard]] VkExtent2D queryFramebufferSize( Context const & Ctx ) noexcept
  {
    auto Width  = 0;
    auto Height = 0;

    do
    {
      glfwGetFramebufferSize( Ctx.Window, &Width, &Height );
      glfwWaitEvents();
    } while ( Width == 0 || Height == 0 );

    return { static_cast< uint32_t >( Width ), static_cast< uint32_t >( Height ) };
  }

}  // namespace mvk::engine
