#ifndef MVK_UTILITY_META_CONDITION_HPP_INCLUDED
#define MVK_UTILITY_META_CONDITION_HPP_INCLUDED

#include <type_traits>

namespace mvk::utility::meta
{

// clang-format off

template <typename T>
struct same_as_impl
{
  template <typename U>
  static constexpr auto 
  check(U) noexcept
  {
    return std::false_type{};
  }

  static constexpr auto
  check(T) noexcept
  {
    return std::true_type{};
  }
};

template <typename T>
constexpr auto 
same_as()
{
  return same_as_impl<T>{};
}

template <template <typename> typename Tag>
struct tagged_with_impl_1
{
  template <typename U>
  static constexpr auto 
  check(U) noexcept
  {
    return std::false_type{};
  }

	template <typename U>
  static constexpr auto 
  check(Tag<U>) noexcept
  {
    return std::true_type{};
  }
};


template <template <auto> typename Tag>
struct tagged_with_impl_2
{
  template <typename U>
  static constexpr auto 
  check(U) noexcept
  {
    return std::false_type{};
  }

  template <auto V>
  static constexpr auto 
  check(Tag<V>) noexcept
  {
    return std::true_type{};
  }
};

template <template <typename> typename Tag>
constexpr auto
tagged_with()
{
	return tagged_with_impl_1<Tag>{};
}

template <template <auto> typename Tag>
constexpr auto
tagged_with()
{
	return tagged_with_impl_2<Tag>{};
}

// clang-format on

} // namespace mvk::utility::meta

#endif
