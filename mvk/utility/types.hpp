#ifndef MVK_UTILITY_TYPES_HPP_INCLUDED
#define MVK_UTILITY_TYPES_HPP_INCLUDED

#include <iterator>
#include <type_traits>
#include <utility>

namespace mvk::utility
{

template <typename Other, typename This>
using is_not_this = std::bool_constant<!std::is_same_v<Other, std::decay_t<This>> && !std::is_base_of_v<Other, std::decay_t<This>>>;

template <typename T, typename = void, typename = void>
struct has_data_and_size : std::false_type
{
};

template <typename T>
struct has_data_and_size<T, std::void_t<decltype(std::data(std::declval<T>()))>, std::void_t<decltype(std::size(std::declval<T>()))>> : std::true_type
{
};

template <typename T>
static constexpr inline auto has_data_and_size_v = has_data_and_size<T>::value;

namespace detail
{

template <typename T>
using value_type_from_data_t = std::remove_pointer_t<decltype(std::data(std::declval<T &>()))>;

} // namespace detail

template <typename T>
[[nodiscard]] static constexpr std::byte const *
force_cast_to_byte(T const * data) noexcept
{
        return static_cast<std::byte const *>(static_cast<void const *>(data));
}

// Get the type by value if it's trivially copyable and constructible, else
// get the reference
template <typename T, bool = std::is_trivial_v<T>>
struct const_trivial
{
        using type = std::remove_cvref_t<T>;
};

template <typename T>
struct const_trivial<T, false>
{
        using type = std::remove_cvref_t<T> const &;
};

template <typename T>
using const_trivial_t = typename const_trivial<T>::type;

template <typename T, bool = std::is_trivial_v<T>>
struct trivial
{
        using type = std::remove_cvref_t<T>;
};

template <typename T>
struct trivial<T, false>
{
        using type = std::remove_cvref_t<T> &;
};

template <typename T>
using trivial_t = typename trivial<T>::type;

template <typename From, typename To>
static constexpr inline auto is_convertible_as_array_v = std::is_convertible_v<From (*)[], To (*)[]>;

template <typename Container, typename Element, typename = void>
struct is_compatible_with_element : std::false_type
{
};

template <typename Container, typename Element>
struct is_compatible_with_element<Container, Element, std::void_t<decltype(std::data(std::declval<Container>()))>>
        : std::bool_constant<is_convertible_as_array_v<detail::value_type_from_data_t<Container>, Element>>
{
};

template <typename Container, typename Element>
static constexpr inline bool is_compatible_with_element_v = is_compatible_with_element<Container, Element>::value;

template <typename Iterator>
using get_iterator_category_t = typename std::iterator_traits<Iterator>::iterator_category;

template <typename Iterator>
using is_random_access = std::is_same<get_iterator_category_t<Iterator>, std::random_access_iterator_tag>;

template <typename Iterator>
static constexpr inline bool is_random_access_v = is_random_access<Iterator>::value;

namespace detail
{

template <typename Iterator>
[[nodiscard]] constexpr decltype(&(*std::declval<Iterator>()))
unwrap_iterator(Iterator const iterator) noexcept
{
        return &(*iterator);
}

template <typename Iterator>
using value_type_from_iterator_t = std::remove_pointer_t<decltype(unwrap_iterator(std::declval<Iterator>()))>;

} // namespace detail

} // namespace mvk::utility

#endif
