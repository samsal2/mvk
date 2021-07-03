#include "engine/context.hpp"

namespace mvk::engine
{
  [[nodiscard]] float current_time( context const & ctx ) noexcept
  {
    auto const current_time = std::chrono::high_resolution_clock::now();
    auto const delta_time   = current_time - ctx.start_time;
    return std::chrono::duration< float, std::chrono::seconds::period >( delta_time ).count();
  }

}  // namespace mvk::engine
