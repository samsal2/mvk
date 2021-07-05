#ifndef MVK_UTILITY_TYPES_HPP_INCLUDED
#define MVK_UTILITY_TYPES_HPP_INCLUDED

#include <iterator>
#include <type_traits>
#include <utility>

namespace mvk::utility
{
  template< typename Other, typename This >
  using IsNotThis = std::bool_constant< !std::is_same_v< std::decay_t< Other >, This > &&
                                        !std::is_base_of_v< This, std::decay_t< Other > > >;

  template< typename T, typename = void, typename = void >
  struct HasDataAndSizeImpl : std::false_type
  {};

  template< typename T >
  struct HasDataAndSizeImpl< T,
                             std::void_t< decltype( std::data( std::declval< T >() ) ) >,
                             std::void_t< decltype( std::size( std::declval< T >() ) ) > > : std::true_type
  {};

  template< typename T >
  static constexpr inline auto HasDataAndSize = HasDataAndSizeImpl< T >::value;

  namespace detail
  {
    template< typename T >
    using ValueTypeFromData = std::remove_pointer_t< decltype( std::data( std::declval< T & >() ) ) >;

  }  // namespace detail

  template< typename T >
  [[nodiscard]] static constexpr std::byte const * forceCastToByte( T const * Data ) noexcept
  {
    return static_cast< std::byte const * >( static_cast< void const * >( Data ) );
  }

  template< typename T >
  [[nodiscard]] static constexpr std::byte * forceCastToByte( T * Data ) noexcept
  {
    return static_cast< std::byte * >( static_cast< void * >( Data ) );
  }

  template< typename From, typename To >
  static constexpr inline auto IsConvertibleAsArrayTo = std::is_convertible_v< From ( * )[], To ( * )[] >;

  template< typename Container, typename Elem, typename = void >
  struct IsCompatWithElemImpl : std::false_type
  {};

  template< typename Container, typename Elem >
  struct IsCompatWithElemImpl< Container, Elem, std::void_t< decltype( std::data( std::declval< Container >() ) ) > >
    : std::bool_constant< IsConvertibleAsArrayTo< detail::ValueTypeFromData< Container >, Elem > >
  {};

  template< typename Container, typename Element >
  static constexpr inline bool IsCompatibleWithElement = IsCompatWithElemImpl< Container, Element >::value;

  template< typename It >
  using GetItCategory = typename std::iterator_traits< It >::iterator_category;

  template< typename It >
  static constexpr inline auto IsRandomAccess = std::is_same_v< GetItCategory< It >, std::random_access_iterator_tag >;

  namespace detail
  {
    template< typename It >
    [[nodiscard]] constexpr decltype( &( *std::declval< It >() ) ) unwrapIt( It const Iter ) noexcept
    {
      return &( *Iter );
    }

    template< typename It >
    using ValueTypeFromIt = std::remove_pointer_t< decltype( unwrapIt( std::declval< It >() ) ) >;

  }  // namespace detail

  template< typename T >
  using Uncvref = std::remove_cvref_t< T >;

  template< typename T >
  static constexpr inline auto AlwaysFalse = false;

}  // namespace mvk::utility

#endif
