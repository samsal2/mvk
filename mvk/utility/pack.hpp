#ifndef MVK_UTILITY_DETAIL_PACK_HPP_INCLUDED
#define MVK_UTILITY_DETAIL_PACK_HPP_INCLUDED

#include <cstddef>
#include <type_traits>

namespace mvk::utility
{
  template < size_t Size >
  using size_constant = std::integral_constant< size_t, Size >;

  struct none
  {};

  template < typename T >
  constexpr auto is_none( [[maybe_unused]] T type ) noexcept
  {
    return std::false_type{};
  }

  constexpr auto is_none( [[maybe_unused]] none type ) noexcept
  {
    return std::true_type{};
  }

  constexpr auto inverse( [[maybe_unused]] std::false_type type ) noexcept
  {
    return std::true_type{};
  }

  constexpr auto inverse( [[maybe_unused]] std::true_type type ) noexcept
  {
    return std::false_type{};
  }

  template < template < typename > typename Tag, typename T >
  constexpr auto unpack_tag( [[maybe_unused]] Tag< T > tag ) noexcept
  {
    return T{};
  }

  template < typename T >
  constexpr auto unpack_tag( [[maybe_unused]] T type ) noexcept
  {
    return none{};
  }

  template < template < auto > typename Tag, auto V >
  constexpr auto unpack_tag( [[maybe_unused]] Tag< V > tag ) noexcept
  {
    return V;
  }

  template < typename T >
  concept not_none = requires
  {
    requires !std::is_same_v< T, none >;
  };

  template < typename Then, typename Else >
  constexpr auto
    if_helper( [[maybe_unused]] std::true_type condition, Then is_true, [[maybe_unused]] Else is_false ) noexcept
  {
    return is_true;
  }

  template < typename Then, typename Else >
  constexpr auto
    if_helper( [[maybe_unused]] std::false_type condition, [[maybe_unused]] Then is_true, Else is_false ) noexcept
  {
    return is_false;
  }

  template < typename Tag >
  constexpr auto is_tag( Tag tag ) noexcept
  {
    return inverse( is_none( unpack_tag( tag ) ) );
  }

  template < typename... Ts >
  struct pack
  {};

  template < typename... Ts >
  constexpr auto size( [[maybe_unused]] pack< Ts... > elements ) noexcept
  {
    return size_constant< sizeof...( Ts ) >{};
  }

  template < typename... Rhs, typename... Lhs >
  constexpr auto concat() noexcept
  {
    return pack< Rhs..., Lhs... >{};
  }

  template < typename... Ts >
  constexpr auto is_empty( [[maybe_unused]] pack< Ts... > elements ) noexcept
  {
    return std::false_type{};
  }

  constexpr auto is_empty( [[maybe_unused]] pack<> elements ) noexcept
  {
    return std::true_type{};
  }

  template < typename T, typename... Ts >
  constexpr auto pop_front( [[maybe_unused]] pack< T, Ts... > elements ) noexcept
  {
    return pack< Ts... >{};
  };

  template < typename T, typename... Ts >
  constexpr auto first( [[maybe_unused]] pack< T, Ts... > elements ) noexcept
  {
    return T{};
  };

  template < typename Condition >
  constexpr auto find_if( [[maybe_unused]] Condition condition, [[maybe_unused]] pack<> elements )
  {
    return none{};
  }

  template < typename Condition, typename... Ts >
  constexpr auto find_if( [[maybe_unused]] Condition condition, pack< Ts... > elements )
  {
    auto current = first( elements );
    auto check   = Condition::check( current );
    return if_helper( check, current, find_if( condition, pop_front( elements ) ) );
  }

  template < typename Condition, typename... Ts >
  constexpr auto exists( pack< Ts... > p, Condition condition ) noexcept
  {
    return inverse( is_none( find_if( p, condition ) ) );
  }

  template < typename This >
  struct matches_impl
  {
    template < typename Other >
    static constexpr auto check( [[maybe_unused]] Other type ) noexcept
    {
      return std::false_type{};
    }

    static constexpr auto check( [[maybe_unused]] This type ) noexcept
    {
      return std::true_type{};
    }
  };

  template < typename This >
  constexpr auto matches()
  {
    return matches_impl< This >{};
  }

  template < template < typename > typename Tag >
  struct tagged_with_impl_1
  {
    template < typename NotTagged >
    static constexpr auto check( [[maybe_unused]] NotTagged type ) noexcept
    {
      return std::false_type{};
    }

    template < typename T >
    static constexpr auto check( [[maybe_unused]] Tag< T > type ) noexcept
    {
      return std::true_type{};
    }
  };

  template < template < auto > typename Tag >
  struct tagged_with_impl_2
  {
    template < typename NotTagged >
    static constexpr auto check( [[maybe_unused]] NotTagged type ) noexcept
    {
      return std::false_type{};
    }

    template < auto V >
    static constexpr auto check( [[maybe_unused]] Tag< V > type ) noexcept
    {
      return std::true_type{};
    }
  };

  template < template < typename > typename Tag >
  constexpr auto tagged_with()
  {
    return tagged_with_impl_1< Tag >{};
  }

  template < template < auto > typename Tag >
  constexpr auto tagged_with()
  {
    return tagged_with_impl_2< Tag >{};
  }

};  // namespace mvk::utility

#endif
