#ifndef MVK_UTILITY_CONCEPTS_HPP_INCLUDED
#define MVK_UTILITY_CONCEPTS_HPP_INCLUDED

#include "utility/types.hpp"

#include <concepts>
#include <type_traits>

namespace mvk::utility
{
  template <typename This, typename That>
  concept SameAs = requires
  {
    requires std::is_same_v<This, That>;
    requires std::is_same_v<That, This>;
  };

  template <typename T>
  concept Trivial = requires
  {
    requires std::is_trivial_v<T>;
  };

  template <typename Other, typename This>
  concept NotThis = requires
  {
    requires !std::is_same_v<std::decay_t<Other>, This>;
    requires !std::is_base_of_v<This, std::decay_t<Other>>;
  };

  template <typename Container>
  concept WithData = requires(Container Cntr)
  {
    { std::data(Cntr) };
  };

  template <typename Container>
  concept WithSize = requires(Container Cntr)
  {
    { std::size(Cntr) };
  };

  template <typename Container>
  concept WithDataAndSize = requires
  {
    requires WithData<Container>;
    requires WithSize<Container>;
  };

  namespace detail
  {
    template <typename Container>
    using ValueTypeFromData = std::remove_pointer_t<decltype(std::data(std::declval<Container &>()))>;

  };  // namespace detail

  template <typename From, typename To>
  concept ConvertibleAsArrayTo = requires
  {
    requires std::is_convertible_v<From(*)[], To(*)[]>;
  };

  template <typename Container, typename Element, typename Value = detail::ValueTypeFromData<Container>>
  concept CompatibleWithElement = requires
  {
    requires WithData<Container>;
    requires ConvertibleAsArrayTo<Value, Element>;
  };

  namespace detail
  {
    template <typename It, typename ItTag>
    concept MatchItTag = requires
    {
      requires std::is_same_v<GetItCategory<It>, ItTag>;
    };

  }  // namespace detail

  template <typename It>
  concept RandomAccess = requires
  {
    requires detail::MatchItTag<It, std::random_access_iterator_tag>;
  };

  template <typename Number>
  concept Integral = requires
  {
    requires std::is_integral_v<Number>;
  };

}  // namespace mvk::utility

#endif
