#ifndef MVK_UTILITY_DETAIL_COMMON_HPP_INCLUDED
#define MVK_UTILITY_DETAIL_COMMON_HPP_INCLUDED

#include <cstddef>
#include <type_traits>

namespace mvk::utility::detail
{

template <size_t Size>
using size_constant = std::integral_constant<size_t, Size>;

struct none
{
};

template <typename T>
constexpr auto
is_none([[maybe_unused]] T type) noexcept
{
  return std::false_type{};
}

constexpr auto
is_none([[maybe_unused]] none type) noexcept
{
  return std::true_type{};
}

constexpr auto
inverse([[maybe_unused]] std::false_type type) noexcept
{
  return std::true_type{};
}

constexpr auto
inverse([[maybe_unused]] std::true_type type) noexcept
{
  return std::false_type{};
}

template <template <typename> typename Tag, typename T>
constexpr auto
unpack_tag([[maybe_unused]] Tag<T> tag) noexcept
{
  return T{};
}

template <typename T>
constexpr auto
unpack_tag([[maybe_unused]] T type) noexcept
{
  return none{};
}

template <template <auto> typename Tag, auto V>
constexpr auto
unpack_tag([[maybe_unused]] Tag<V> tag) noexcept
{
  return V;
}

template <typename T>
concept not_none = requires
{
  requires !std::is_same_v<T, none>;
};

template <typename Then, typename Else>
constexpr auto
if_helper([[maybe_unused]] std::true_type condition, Then is_true,
          [[maybe_unused]] Else is_false) noexcept
{
  return is_true;
}

template <typename Then, typename Else>
constexpr auto
if_helper([[maybe_unused]] std::false_type consdition,
          [[maybe_unused]] Then is_true, Else is_false) noexcept
{
  return is_false;
}

template <typename Tag>
constexpr auto
is_tag(Tag tag) noexcept
{
  return inverse(is_none(unpack_tag(tag)));
}

} // namespace mvk::utility::detail

#endif
