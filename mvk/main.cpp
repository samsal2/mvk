#include "engine/init.hpp"
#include "engine/misc.hpp"

int main()
{
  auto ctx = mvk::engine::create_context( "stan loona", { 600, 600 } );
  mvk::engine::test_run( ctx );

  return 0;
}
