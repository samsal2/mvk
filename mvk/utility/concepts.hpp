#ifndef MVK_UTILITY_CONCEPTS_HPP_INCLUDED
#define MVK_UTILITY_CONCEPTS_HPP_INCLUDED

#include "utility/types.hpp"

#include <concepts>

namespace mvk::utility
{

template <typename This, typename That>
concept same_as = requires
{
  {std::is_same_v<This, That>};
  {std::is_same_v<That, This>};
};

template <typename Other, typename This>
concept not_this = requires
{
  {!std::is_same_v<Other, std::decay_t<This>>};
  {!std::is_base_of_v<Other, std::decay_t<This>>};
};

template <typename Container>
concept with_data = requires(Container container)
{
  {std::data(container)};
};

template <typename Container>
concept with_size = requires(Container container)
{
  {std::size(container)};
};

template <typename Container>
concept with_data_and_size = requires
{
  {with_data<Container>};
  {with_size<Container>};
};

namespace detail
{
  // clang-format off

template <typename Container>
using value_type_from_data_t =
  std::remove_pointer_t<decltype(std::data(std::declval<Container &>()))>;

  // clang-format on

}; // namespace detail

template <typename From, typename To>
concept convertible_as_array_to = requires
{
  {std::is_convertible_v<From(*)[], To(*)[]>};
};

template <
  typename Container,
  typename Element,
  typename Value = detail::value_type_from_data_t<Container>>
concept compatible_with_element = requires
{
  {with_data<Container>};
  {convertible_as_array_to<Value, Element>};
};

namespace detail
{
  // clang-format off

template <typename Iterator, typename IteratorTag>
concept match_iterator_tag = requires
{
  {std::is_same_v<
    typename std::iterator_traits<Iterator>::iterator_category,
    IteratorTag>};
};

  // clang-format on
} // namespace detail

template <typename Iterator>
concept random_access = requires
{
  {detail::match_iterator_tag<Iterator, std::random_access_iterator_tag>};
};

template <typename Callable, typename Returns, typename... Args>
concept callable = requires(Callable && callable, Args &&... args)
{
  {
    std::forward<Callable>(callable)(std::forward<Args>(args)...)
    } -> same_as<Returns>;
};

} // namespace mvk::utility

#endif
