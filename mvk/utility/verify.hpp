#ifndef MVK_UTILITY_COMMON_HPP
#define MVK_UTILITY_COMMON_HPP

#include "utility/trace.hpp"

#include <string>

namespace mvk::utility
{

[[noreturn]] void
verify_failed(std::string_view file, int line, std::string_view function);

#ifndef NDEBUG
#define MVK_VERIFY(expression)                                               \
  (static_cast<bool>((expression))                                           \
       ? void(0)                                                             \
       : ::mvk::utility::verify_failed(__FILE__, __LINE__,                   \
                                       __PRETTY_FUNCTION__))
#else
#define MVK_VERIFY(expression) void(0)
#endif

#define MVK_VERIFY_NOT_REACHED() MVK_VERIFY(false)

} // namespace mvk::utility

#endif
