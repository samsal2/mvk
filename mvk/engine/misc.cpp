#include "engine/misc.hpp"

#include "engine/draw.hpp"

namespace mvk::engine
{
  void test_run( context & ctx ) noexcept
  {
    while ( !ctx.window_.should_close() )
    {
      glfwPollEvents();

      begin_draw( ctx );

      auto const pvm = create_test_pvm( ctx );

      basic_draw(
        ctx, utility::as_bytes( ctx.vertices_ ), utility::as_bytes( ctx.indices_ ), utility::as_bytes( pvm ) );

      end_draw( ctx );
    }

    vkDeviceWaitIdle( types::get( ctx.device_ ) );
  }

  [[nodiscard]] pvm create_test_pvm( context const & ctx ) noexcept
  {
    auto const time = current_time( ctx );

    constexpr auto turn_rate = glm::radians( 90.0F );

    auto ubo = pvm();

    ubo.model = glm::rotate( glm::mat4( 1.0F ), time * turn_rate, glm::vec3( 0.0F, 0.0F, 1.0F ) );
    ubo.view =
      glm::lookAt( glm::vec3( 2.0F, 2.0F, 2.0F ), glm::vec3( 0.0F, 0.0F, 0.0F ), glm::vec3( 0.0F, 0.0F, 1.0F ) );

    auto const ratio =
      static_cast< float >( ctx.swapchain_extent_.width ) / static_cast< float >( ctx.swapchain_extent_.height );

    ubo.proj = glm::perspective( glm::radians( 45.0F ), ratio, 0.1F, 10.0F );
    ubo.proj[ 1 ][ 1 ] *= -1;

    return ubo;
  }

}  // namespace mvk::engine
