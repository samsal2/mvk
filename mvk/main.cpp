#include "renderer.hpp"
#include "utility/verify.hpp"

int
main()
{
  auto rdr = mvk::renderer();
  rdr.init();
  rdr.run();

  return 0;
}
