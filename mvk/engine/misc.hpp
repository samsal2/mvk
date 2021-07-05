#ifndef MVK_ENGINE_MISC_HPP_INCLUDED
#define MVK_ENGINE_MISC_HPP_INCLUDED

#include "engine/Context.hpp"

namespace mvk::engine
{
  [[nodiscard]] pvm createTestPvm( Context const & Ctx ) noexcept;

  void testRun( Context & Ctx ) noexcept;

}  // namespace mvk::engine

#endif
