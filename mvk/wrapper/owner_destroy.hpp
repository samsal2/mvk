#ifndef MVK_WRAPPER_OWNER_DESTROY_HPP_INCLUDED
#define MVK_WRAPPER_OWNER_DESTROY_HPP_INCLUDED

#include "wrapper/fwd.hpp"

namespace mvk::wrapper
{
  namespace deleter
  {
    struct owner_destroy
    {};

  }  // namespace deleter

  template < auto Call >
  class owner_destroy
  {
    static constexpr auto deleter_call = Call;

  public:
    constexpr owner_destroy() noexcept = default;

    template < typename Handle >
    constexpr void destroy( Handle handle )
    {
      deleter_call( handle, nullptr );
    }
  };

  template < typename... Args >
  constexpr auto deleter_selector( [[maybe_unused]] deleter::owner_destroy option ) noexcept
  {
    constexpr auto deleter_call = select< options::deleter_call >( Args{}... );
    static_assert( !utility::is_none( deleter_call ), "Expected deleter_call option" );

    return selected< owner_destroy< deleter_call > >{};
  }

}  // namespace mvk::wrapper

#endif
