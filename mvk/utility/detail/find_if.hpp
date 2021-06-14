#ifndef MVK_UTILITY_DETAIL_FIND_IF_HPP_INCLUDED
#define MVK_UTILITY_DETAIL_FIND_IF_HPP_INCLUDED

#include "utility/detail/pack.hpp"

namespace mvk::utility::detail
{

template <typename Condition>
struct find_if_impl
{

  template <typename Current>
  static constexpr auto
  helper([[maybe_unused]] pack<> elements, Current type)
  {
    auto check = Condition::check(type);
    return if_helper(check, type, none{});
  }

  template <typename Current, typename T, typename... Ts>
  static constexpr auto
  helper([[maybe_unused]] pack<T, Ts...> elements, Current type)
  {
    auto check = Condition::check(type);
    return if_helper(check, type, helper(pack<Ts...>{}, T{}));
  }

  template <typename T, typename... Ts>
  static constexpr auto
  apply(pack<T, Ts...> elements)
  {
    return helper(elements, T{});
  }
};

template <typename Condition, typename... Ts>
constexpr auto
find_if(pack<Ts...> elements, [[maybe_unused]] Condition condition)
{
  return find_if_impl<Condition>::apply(elements);
}

} // namespace mvk::utility::detail

#endif
