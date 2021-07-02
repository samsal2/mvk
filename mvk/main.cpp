#include "context.hpp"
#include "utility/verify.hpp"

int main()
{
  auto ctx = mvk::create_context( "stan loona", { 600, 600 } );
  mvk::test_run( ctx );

  return 0;
}
