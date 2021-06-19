#ifndef MVK_UTILITY_DETAIL_PACK_HPP_INCLUDED
#define MVK_UTILITY_DETAIL_PACK_HPP_INCLUDED

#include "utility/detail/common.hpp"
#include "utility/detail/condition.hpp"

namespace mvk::utility::detail
{

template <typename... Ts>
struct pack
{
};

template <typename... Ts>
constexpr auto
size([[maybe_unused]] pack<Ts...> elements) noexcept
{
  return size_constant<sizeof...(Ts)>{};
}

template <typename... Rhs, typename... Lhs>
constexpr auto
concat() noexcept
{
  return pack<Rhs..., Lhs...>{};
}

template <typename... Ts>
constexpr auto
is_empty([[maybe_unused]] pack<Ts...> elements) noexcept
{
  return std::false_type{};
}

constexpr auto
is_empty([[maybe_unused]] pack<> elements) noexcept
{
  return std::true_type{};
}

template <typename T, typename... Ts>
constexpr auto
pop_front([[maybe_unused]] pack<T, Ts...> elements) noexcept
{
  return pack<Ts...>{};
};

template <typename T, typename... Ts>
constexpr auto
first([[maybe_unused]] pack<T, Ts...> elements) noexcept
{
  return T{};
};

}; // namespace mvk::utility::detail

#endif
