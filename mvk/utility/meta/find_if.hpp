#ifndef MVK_UTILITY_META_FIND_IF_HPP_INCLUDED
#define MVK_UTILITY_META_FIND_IF_HPP_INCLUDED

#include "utility/meta/pack.hpp"

namespace mvk::utility::meta
{

template <typename Condition>
struct find_if_impl
{
  // Simple check alias for the condition
  template <typename T>
  static constexpr auto
  check(T type)
  {
    return Condition::check(type);
  }

  // If the condition is true, return current
  template <typename Current, typename... Ts>
  static constexpr auto helper(pack<Ts...>, Current, std::true_type)
  {
    return Current{};
  }

  // If the condition is false, go next
  template <typename Current, typename Next, typename... Ts>
  static constexpr auto helper(pack<Next, Ts...>, Current, std::false_type)
  {
    auto const next = Next{};
    return helper(pack<Ts...>{}, next, check(next));
  }

  // If nothing was found, return none
  template <typename Current>
  static constexpr auto helper(pack<>, Current, std::false_type)
  {
    return none{};
  }

  template <typename T, typename... Ts>
  static constexpr auto apply(pack<T, Ts...>)
  {
    return helper(pack<Ts...>{}, T{}, check(T{}));
  }
};

template <typename Condition, typename... Ts>
constexpr auto
find_if(pack<Ts...> p, Condition)
{
  return find_if_impl<Condition>::apply(p);
}

} // namespace mvk::utility::meta

#endif
