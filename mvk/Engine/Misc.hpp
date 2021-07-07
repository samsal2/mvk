#pragma once

#include "Engine/Context.hpp"

namespace Mvk::Engine {

[[nodiscard]] PVM createTestPvm(Renderer const &Render) noexcept;

void testRun(Renderer &Render) noexcept;

} // namespace Mvk::Engine

