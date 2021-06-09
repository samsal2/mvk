#include "renderer.hpp"
#include "utility/verify.hpp"

int
main()
{
  try
  {
    constexpr auto width  = 600;
    constexpr auto height = 600;

    auto rdr = mvk::renderer(width, height);
    rdr.run();
  }
  catch (mvk::utility::verify_error error)
  {
    std::cerr << error.what() << '\n';
  }

  return 0;
}
