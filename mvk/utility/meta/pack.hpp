#ifndef MVK_UTILITY_META_PACK_HPP_INCLUDED
#define MVK_UTILITY_META_PACK_HPP_INCLUDED

#include "utility/meta/common.hpp"
#include "utility/meta/condition.hpp"

namespace mvk::utility::meta
{

template <typename... Ts>
struct pack
{
};

template <typename... Ts>
constexpr auto size(pack<Ts...>) noexcept
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
constexpr auto is_empty(pack<Ts...>) noexcept
{
  return std::false_type{};
}

constexpr auto is_empty(pack<>) noexcept
{
  return std::true_type{};
}

template <typename T, typename... Ts>
constexpr auto pop_front(pack<T, Ts...>) noexcept
{
  return pack<Ts...>{};
};

template <typename T, typename... Ts>
constexpr auto first(pack<T, Ts...>) noexcept
{
  return T{};
};

}; // namespace mvk::utility::meta

#endif
