#ifndef MVK_UTILITY_DETAIL_FIND_IF_HPP_INCLUDED
#define MVK_UTILITY_DETAIL_FIND_IF_HPP_INCLUDED

#include "utility/pack.hpp"

namespace mvk::utility
{

template <typename Condition>
constexpr auto
find_if([[maybe_unused]] Condition condition,
        [[maybe_unused]] pack<> elements)
{
  return none{};
}

template <typename Condition, typename... Ts>
constexpr auto
find_if([[maybe_unused]] Condition condition, pack<Ts...> elements)
{
  auto current = first(elements);
  auto check = Condition::check(current);
  return if_helper(check, current, find_if(condition, pop_front(elements)));
}

template <typename Condition, typename T, typename... Ts>
constexpr auto
find_if([[maybe_unused]] Condition condition, T && current, Ts &&... others)
{
  auto check = Condition::check(current);
  return if_helper(check, std::forward<T>(current),
                   find_if(condition, std::forward<Ts>(others)...));
}

template <typename Condition>
constexpr auto
find_if([[maybe_unused]] Condition condition)
{
  return none{};
}

} // namespace mvk::utility

#endif
