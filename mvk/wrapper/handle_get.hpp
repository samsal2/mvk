#ifndef MVK_WRAPPER_HANDLE_GET_HPP_INCLUDED
#define MVK_WRAPPER_HANDLE_GET_HPP_INCLUDED

#include "wrapper/fwd.hpp"

namespace mvk::wrapper
{
  namespace creator
  {
    struct handle_get
    {};

  }  // namespace creator

  template <auto Call, typename Wrapper>
  class handle_get;

  template <typename... Args>
  constexpr auto creator_selector( [[maybe_unused]] creator::handle_get option ) noexcept
  {
    using wrapper       = any_wrapper<Args...>;
    constexpr auto call = select<options::creator_call>( Args{}... );
    static_assert( !utility::is_none( call ), "Expected creator_call option" );

    return detail::select<handle_get<call, wrapper>>{};
  }

  template <typename Wrapper>
  class handle_get<vkGetDeviceQueue, Wrapper>
  {
    using wrapper_type             = Wrapper;
    static constexpr auto get_call = vkGetDeviceQueue;

  public:
    [[nodiscard]] static constexpr wrapper_type retrieve( VkDevice const device, uint32_t const index ) noexcept
    {
      auto handle = VkQueue();
      get_call( device, index, 0, &handle );
      return wrapper_type( handle );
    }
  };

}  // namespace mvk::wrapper

#endif
