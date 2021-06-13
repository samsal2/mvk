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

static_assert(exists(pack<int, float>{}, same_as<int>()));
static_assert(exists(pack<int, float>{}, same_as<float>()));
static_assert(!exists(pack<int, float>{}, same_as<double>()));

} // namespace mvk::utility::meta

#endif
