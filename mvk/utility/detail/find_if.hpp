#ifndef MVK_UTILITY_DETAIL_FIND_IF_HPP_INCLUDED
#define MVK_UTILITY_DETAIL_FIND_IF_HPP_INCLUDED

#include "utility/detail/pack.hpp"

namespace mvk::utility::detail
{

template <typename Condition>
struct find_if_impl
{

  template <typename Current>
  static constexpr auto helper(pack<>, Current)
  {
    auto check = Condition::check(Current{});
    return if_helper(check, Current{}, none{});
  }

  template <typename Current, typename T, typename... Ts>
  static constexpr auto helper(pack<T, Ts...>, Current)
  {
    auto check = Condition::check(Current{});
    return if_helper(check, Current{}, helper(pack<Ts...>{}, T{}));
  }

  template <typename T, typename... Ts>
  static constexpr auto apply(pack<T, Ts...>)
  {
    return helper(pack<Ts...>{}, T{});
  }
};

template <typename Condition, typename... Ts>
constexpr auto
find_if(pack<Ts...> p, Condition)
{
  return find_if_impl<Condition>::apply(p);
}

} // namespace mvk::utility::detail

#endif
