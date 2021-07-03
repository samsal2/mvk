#ifndef MVK_ENGINE_DRAW_HPP_INCLUDED
#define MVK_ENGINE_DRAW_HPP_INCLUDED

#include "engine/context.hpp"

namespace mvk::engine
{
  void begin_draw( context & ctx ) noexcept;

  void basic_draw( context &                         ctx,
                   utility::slice< std::byte const > vertices,
                   utility::slice< std::byte const > indices,
                   utility::slice< std::byte const > pvm ) noexcept;

  void end_draw( context & ctx ) noexcept;

  void recreate_after_swapchain_change( context & ctx ) noexcept;

}  // namespace mvk::engine
#endif
