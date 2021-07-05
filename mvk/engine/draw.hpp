#ifndef MVK_ENGINE_DRAW_HPP_INCLUDED
#define MVK_ENGINE_DRAW_HPP_INCLUDED

#include "engine/Context.hpp"

namespace mvk::engine
{
  void beginDraw( Context & Ctx ) noexcept;

  void basicDraw( Context &                         Ctx,
                  utility::Slice< std::byte const > Vtxs,
                  utility::Slice< std::byte const > Idxs,
                  utility::Slice< std::byte const > Pvm ) noexcept;

  void endDraw( Context & Ctx ) noexcept;

  void recreateAfterSwapchainChange( Context & Ctx ) noexcept;

}  // namespace mvk::engine
#endif
