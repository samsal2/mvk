#pragma once

#include <cassert>

namespace Mvk::Utility
{
#ifndef NDEBUG
#  define MVK_VERIFY( expression ) assert( expression )
#else
#  define MVK_VERIFY( expression ) (void)( expression )
#endif

#define MVK_VERIFY_NOT_REACHED() MVK_VERIFY( false )

}  // namespace Mvk::Utility
