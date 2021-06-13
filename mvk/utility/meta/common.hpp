#ifndef MVK_UTILITY_META_COMMON_HPP_INCLUDED
#define MVK_UTILITY_META_COMMON_HPP_INCLUDED

#include <cstddef>
#include <type_traits>

namespace mvk::utility::meta
{

// clang-format off

template <size_t Size>
using size_constant = std::integral_constant<size_t, Size>;

struct none
{
};

template <typename U>
constexpr auto
is_none(U) noexcept
{
	return std::false_type{};
}

constexpr auto
is_none(none) noexcept
{
	return std::true_type{};
}

constexpr auto
inverse(std::false_type) noexcept
{
	return std::true_type{};
}

constexpr auto
inverse(std::true_type) noexcept
{
	return std::false_type{};
}

template <template <typename> typename Tag, typename T>
constexpr auto
unpack_tag(Tag<T>)
{
	return T{};
}

template <typename T>
constexpr auto
unpack_tag(T)
{
	return none{};
}

template <template <auto> typename Tag, auto V>
constexpr auto
unpack_tag(Tag<V>)
{
	return V;
}

template <typename Lhs, typename Rhs>
constexpr void
avoid_none_swap(Lhs & lhs, Rhs & rhs) noexcept
{
	std::swap(lhs, rhs);
}

constexpr void
avoid_none_swap(none, none) noexcept
{
}

template <typename T>
concept not_none = requires
{
	requires !std::is_same_v<T, none>;
};

// clang-format on

} // namespace mvk::utility::meta

#endif