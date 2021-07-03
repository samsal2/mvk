#include "engine/context.hpp"

namespace mvk::engine
{
  [[nodiscard]] float current_time( context const & ctx ) noexcept
  {
    auto const current_time = std::chrono::high_resolution_clock::now();
    auto const delta_time   = current_time - ctx.start_time;
    return std::chrono::duration< float, std::chrono::seconds::period >( delta_time ).count();
  }

  [[nodiscard]] VkExtent2D query_framebuffer_size( context const & ctx ) noexcept
  {
    auto width  = 0;
    auto height = 0;

    do
    {
      glfwGetFramebufferSize( ctx.window, &width, &height );
      glfwWaitEvents();
    } while ( width == 0 || height == 0 );

    return { static_cast< uint32_t >( width ), static_cast< uint32_t >( height ) };
  }

}  // namespace mvk::engine
