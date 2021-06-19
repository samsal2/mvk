#include "utility/verify.hpp"

#include <iostream>

namespace mvk::utility
{

[[noreturn]] void verify_failed(std::string_view file, int line, std::string_view function)
{
        std::cerr << "MVK_VERIFY failure in " << file << ':' << line << " inside " << function << '\n';
        abort();
}

} // namespace mvk::utility
