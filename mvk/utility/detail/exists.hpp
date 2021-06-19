#ifndef MVK_UTILITY_DETAIL_EXISTS_HPP_INCLUDED
#define MVK_UTILITY_DETAIL_EXISTS_HPP_INCLUDED

#include "utility/detail/condition.hpp"
#include "utility/detail/find_if.hpp"
#include "utility/detail/pack.hpp"

namespace mvk::utility::detail
{

template <typename Condition, typename... Ts> constexpr auto exists(pack<Ts...> p, Condition condition) noexcept
{
        return inverse(is_none(find_if(p, condition)));
}

} // namespace mvk::utility::detail

#endif
