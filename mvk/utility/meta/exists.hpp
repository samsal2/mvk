#ifndef MVK_UTILITY_META_EXISTS_HPP_INCLUDED
#define MVK_UTILITY_META_EXISTS_HPP_INCLUDED

#include "utility/meta/condition.hpp"
#include "utility/meta/find_if.hpp"
#include "utility/meta/pack.hpp"

namespace mvk::utility::meta
{

template <typename Condition, typename... Ts>
constexpr auto
exists(pack<Ts...> p, Condition condition) noexcept
{
  return inverse(is_none(find_if(p, condition)));
}

} // namespace mvk::utility::meta

#endif
