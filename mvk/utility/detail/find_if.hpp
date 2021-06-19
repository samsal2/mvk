#ifndef MVK_UTILITY_DETAIL_FIND_IF_HPP_INCLUDED
#define MVK_UTILITY_DETAIL_FIND_IF_HPP_INCLUDED

#include "utility/detail/pack.hpp"

namespace mvk::utility::detail
{

template <typename Condition> constexpr auto find_if([[maybe_unused]] pack<> elements, [[maybe_unused]] Condition condition)
{
        return none{};
}

template <typename Condition, typename... Ts> constexpr auto find_if(pack<Ts...> elements, [[maybe_unused]] Condition condition)
{
        auto current = first(elements);
        auto check   = Condition::check(current);
        return if_helper(check, current, find_if(pop_front(elements), condition));
}

template <typename Condition, typename T, typename... Ts> constexpr auto find_if([[maybe_unused]] Condition condition, T current, Ts... others)
{
        auto check = Condition::check(current);
        return if_helper(check, current, find_if(condition, others...));
}

template <typename Condition> constexpr auto find_if([[maybe_unused]] Condition condition)
{
        return none{};
}

} // namespace mvk::utility::detail

#endif
