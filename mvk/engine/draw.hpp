#ifndef MVK_ENGINE_DRAW_HPP_INCLUDED
#define MVK_ENGINE_DRAW_HPP_INCLUDED

#include "engine/context.hpp"

namespace mvk::engine
{
  void beginDraw(InOut<Context> Ctx) noexcept;

  void basicDraw(InOut<Context>                  Ctx,
                 utility::Slice<std::byte const> Vtxs,
                 utility::Slice<std::byte const> Idxs,
                 utility::Slice<std::byte const> Pvm) noexcept;

  void endDraw(InOut<Context> Ctx) noexcept;

  void recreateAfterSwapchainChange(InOut<Context> Ctx) noexcept;

}  // namespace mvk::engine
#endif
