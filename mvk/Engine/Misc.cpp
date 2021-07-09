#include "Engine/Misc.hpp"

#include "Detail/Misc.hpp"
#include "Detail/Readers.hpp"
#include "Engine/VulkanContext.hpp"

namespace Mvk::Engine
{
  [[nodiscard]] PVM createTestPvm() noexcept
  {
    auto const time = VulkanContext::the().getCurrentTime();

    constexpr auto turn_rate = glm::radians( 90.0F );

    auto ubo = PVM();

    ubo.model = glm::rotate( glm::mat4( 1.0F ), time * turn_rate, glm::vec3( 0.0F, 0.0F, 1.0F ) );
    ubo.view  = glm::lookAt( glm::vec3( 2.0F, 2.0F, 2.0F ), glm::vec3( 0.0F, 0.0F, 0.0F ), glm::vec3( 0.0F, 0.0F, 1.0F ) );

    // TODO(samuel): Ratio
    ubo.proj = glm::perspective( glm::radians( 45.0F ), 1.0F, 0.1F, 10.0F );
    ubo.proj[1][1] *= -1;

    return ubo;
  }

}  // namespace Mvk::Engine
