#ifndef MVK_ENGINE_MISC_HPP_INCLUDED
#define MVK_ENGINE_MISC_HPP_INCLUDED

#include "engine/context.hpp"

namespace mvk::engine
{
  [[nodiscard]] pvm create_test_pvm( context const & ctx ) noexcept;

  void test_run( context & ctx ) noexcept;

}  // namespace mvk::engine

#endif
