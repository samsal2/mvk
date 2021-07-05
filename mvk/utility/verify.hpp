#ifndef MVK_UTILITY_COMMON_HPP
#define MVK_UTILITY_COMMON_HPP

#include <string_view>

namespace mvk::utility
{
  [[noreturn]] void verifyFailed( std::string_view File, int Line, std::string_view Fn );

#ifndef NDEBUG
#  define MVK_VERIFY( expression )          \
    ( static_cast< bool >( ( expression ) ) \
        ? void( 0 )                         \
        : ::mvk::utility::verifyFailed( __FILE__, __LINE__, __PRETTY_FUNCTION__ ) )
#else
#  define MVK_VERIFY( expression ) (void)( expression )
#endif

#define MVK_VERIFY_NOT_REACHED() MVK_VERIFY( false )

}  // namespace mvk::utility

#endif
