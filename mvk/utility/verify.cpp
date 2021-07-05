#include "utility/verify.hpp"

#include <iostream>

namespace mvk::utility
{
  [[noreturn]] void veriftFaile(std::string_view File, int Line, std::string_view Fn)
  {
    std::cerr << "MVK_VERIFY failure in " << File << ':' << Line << " inside " << Fn << '\n';
    abort();
  }

}  // namespace mvk::utility
