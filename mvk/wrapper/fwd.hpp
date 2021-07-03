#ifndef MVK_WRAPPER_FWD_HPP_INCLUDED
#define MVK_WRAPPER_FWD_HPP_INCLUDED

#include "utility/pack.hpp"
#include "utility/types.hpp"

#include <type_traits>
#include <vulkan/vulkan.h>

namespace mvk::wrapper
{
  template < typename... Args >
  class any_wrapper;

  template < typename Found >
  struct selected
  {
    using type = Found;
  };

  template < typename Selected >
  using selected_t = typename Selected::type;

  namespace options
  {
    template < typename Deleter >
    struct deleter
    {};

    template < typename Creator >
    struct creator
    {};

    template < typename Handle >
    struct handle
    {};

    template < typename Parent >
    struct parent
    {};

    template < typename Pool >
    struct pool
    {};

    template < typename Storage >
    struct storage
    {};

    template < auto Call >
    struct deleter_call
    {};

    template < auto Call >
    struct creator_call
    {};

  }  // namespace options

  template < typename T, typename... Args >
  constexpr auto storage_selector( [[maybe_unused]] T t ) noexcept
  {
    static_assert( utility::always_false< T >, "invalid storage option" );
  }

  template < typename T, typename... Args >
  constexpr auto deleter_selector( [[maybe_unused]] T t ) noexcept
  {
    static_assert( utility::always_false< T >, "invalid deleter option" );
  }

  template < template < typename > typename Tag, typename... Args >
  constexpr auto select( [[maybe_unused]] Args... opt )
  {
    return utility::unpack_tag( utility::find_if( utility::tagged_with< Tag >(), utility::pack< Args... >{} ) );
  }

  template < template < auto > typename Tag, typename... Args >
  constexpr auto select( [[maybe_unused]] Args... opt )
  {
    return utility::unpack_tag( utility::find_if( utility::tagged_with< Tag >(), utility::pack< Args... >{} ) );
  }

}  // namespace mvk::wrapper

#endif
