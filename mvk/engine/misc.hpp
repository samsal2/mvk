#ifndef MVK_ENGINE_MISC_HPP_INCLUDED
#define MVK_ENGINE_MISC_HPP_INCLUDED

#include "engine/context.hpp"

namespace mvk::engine
{
  [[nodiscard]] pvm createTestPvm(In<Context> Ctx) noexcept;

  void testRun(InOut<Context> Ctx) noexcept;

}  // namespace mvk::engine

#endif
