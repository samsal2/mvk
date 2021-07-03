#ifndef MVK_WRAPPER_ANY_WRAPPER_HPP_INCLUDED
#define MVK_WRAPPER_ANY_WRAPPER_HPP_INCLUDED

#include "wrapper/handle_only.hpp"
#include "wrapper/object_destroy.hpp"
#include "wrapper/object_free.hpp"
#include "wrapper/owner_destroy.hpp"
#include "wrapper/unique.hpp"

namespace mvk::wrapper
{
  template< typename... Args >
  using any_wrapper_base =
    selected_t< decltype( storage_selector< Args... >( select< options::storage >( Args{}... ) ) ) >;

  template< typename... Args >
  class any_wrapper : public any_wrapper_base< Args... >
  {
  public:
    using any_wrapper_base< Args... >::any_wrapper_base;
  };

  template< typename... LhsArgs, typename... RhsArgs >
  requires utility::same_as< decltype( select< options::handle >( LhsArgs{}... ) ),
                             decltype( select< options::handle >( RhsArgs{}... ) ) >
  constexpr bool operator==( wrapper::any_wrapper< LhsArgs... > const & lhs,
                             wrapper::any_wrapper< RhsArgs... > const & rhs ) noexcept
  {
    return lhs.get() == rhs.get();
  }

  template< typename... Args >
  constexpr bool operator==( wrapper::any_wrapper< Args... > const &            lhs,
                             decltype( select< options::handle >( Args{}... ) ) handle_type ) noexcept
  {
    return lhs.get() == handle_type;
  }

  template< typename... LhsArgs, typename... RhsArgs >
  requires utility::same_as< decltype( select< options::handle >( LhsArgs{}... ) ),
                             decltype( select< options::handle >( RhsArgs{}... ) ) >
  constexpr bool operator!=( wrapper::any_wrapper< LhsArgs... > const & lhs,
                             wrapper::any_wrapper< RhsArgs... > const & rhs ) noexcept
  {
    return lhs.get() != rhs.get();
  }

  template< typename... Args >
  constexpr bool operator!=( wrapper::any_wrapper< Args... > const &            lhs,
                             decltype( select< options::handle >( Args{}... ) ) handle_type ) noexcept
  {
    return lhs.get() != handle_type;
  }

}  // namespace mvk::wrapper

#endif
