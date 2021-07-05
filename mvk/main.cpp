#include "engine/init.hpp"
#include "engine/misc.hpp"

int main()
{
  auto Ctx = mvk::engine::createContext( "stan loona", { 600, 600 } );
  mvk::engine::testRun( Ctx );
  mvk::engine::destroyContext( Ctx );

  return 0;
}
