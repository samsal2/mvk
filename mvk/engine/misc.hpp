#ifndef MVK_ENGINE_MISC_HPP_INCLUDED
#define MVK_ENGINE_MISC_HPP_INCLUDED

#include "Engine/Context.hpp"

namespace Mvk::Engine {

[[nodiscard]] PVM createTestPvm(Renderer const &Render) noexcept;

void testRun(Renderer &Render) noexcept;

} // namespace Mvk::Engine

#endif
