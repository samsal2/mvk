#include "engine/init.hpp"
#include "engine/misc.hpp"

int main()
{
  mvk::engine::Context Ctx;
  mvk::engine::createContext("stan loona", { 600, 600 }, &Ctx);
  mvk::engine::testRun(&Ctx);
  mvk::engine::dtyContext(&Ctx);

  return 0;
}
