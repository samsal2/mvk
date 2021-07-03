#ifndef MVK_UTILITY_COMPRESSED_PAIR_HPP_INCLUDED
#define MVK_UTILITY_COMPRESSED_PAIR_HPP_INCLUDED

#include "utility/concepts.hpp"

#include <cstddef>
#include <type_traits>

namespace mvk::utility
{
  template < size_t Idx, typename T, bool IsEmpty = std::is_empty_v< T > >
  class compressed_pair_leaf : public T
  {
  public:
    using value_type      = T;
    using base            = T;
    using reference       = value_type &;
    using const_reference = value_type const &;

    constexpr compressed_pair_leaf() noexcept = default;

    template < typename U >
    requires utility::not_this< U, compressed_pair_leaf >
    constexpr explicit compressed_pair_leaf( U && value ) noexcept : base( std::forward< U >( value ) ) {}

    [[nodiscard]] constexpr value_type const & get() const noexcept
    {
      return static_cast< value_type const & >( *this );
    }

    [[nodiscard]] constexpr value_type & get() noexcept
    {
      return static_cast< value_type & >( *this );
    }
  };

  template < size_t Idx, typename T >
  class compressed_pair_leaf< Idx, T, false >
  {
  public:
    using value_type      = T;
    using reference       = value_type &;
    using const_reference = value_type const &;

    constexpr compressed_pair_leaf() noexcept = default;

    template < typename U >
    requires utility::not_this< U, compressed_pair_leaf >
    constexpr explicit compressed_pair_leaf( U && value ) noexcept : value_( std::forward< U >( value ) ) {}

    [[nodiscard]] constexpr value_type const & get() const noexcept
    {
      return value_;
    }

    [[nodiscard]] constexpr value_type & get() noexcept
    {
      return value_;
    }

  private:
    value_type value_ = {};
  };

  template < typename T1, typename T2 >
  class compressed_pair
    : protected compressed_pair_leaf< 0, T1 >
    , protected compressed_pair_leaf< 1, T2 >
  {
    static_assert( !std::is_same_v< T1, T2 > );

    using leaf1 = compressed_pair_leaf< 0, T1 >;
    using leaf2 = compressed_pair_leaf< 1, T2 >;

  public:
    constexpr compressed_pair() noexcept = default;

    template < typename U1, typename U2 >
    constexpr compressed_pair( U1 && value1, U2 && value2 ) noexcept
      : leaf1( std::forward< U1 >( value1 ) ), leaf2( std::forward< U2 >( value2 ) )
    {}

    [[nodiscard]] constexpr typename leaf1::const_reference first() const noexcept
    {
      return static_cast< leaf1 const & >( *this ).get();
    }

    [[nodiscard]] constexpr typename leaf1::reference first() noexcept
    {
      return static_cast< leaf1 & >( *this ).get();
    }

    [[nodiscard]] constexpr typename leaf2::const_reference second() const noexcept
    {
      return static_cast< leaf2 const & >( *this ).get();
    }

    [[nodiscard]] constexpr typename leaf2::reference second() noexcept
    {
      return static_cast< leaf2 & >( *this ).get();
    }
  };

}  // namespace mvk::utility

#endif
