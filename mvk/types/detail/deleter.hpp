#ifndef MVK_VK_TYPE_DETAIL_DELETER_HPP_INCLUDED
#define MVK_VK_TYPE_DETAIL_DELETER_HPP_INCLUDED

#include "types/common.hpp"
#include "utility/detail/common.hpp"
#include "utility/misc.hpp"
#include "utility/slice.hpp"

namespace mvk::types::detail
{

template <auto Call, typename Handle>
constexpr void
delete_dispatch(Handle handle, utility::detail::none, utility::detail::none)
{
  Call(handle, nullptr);
}

template <auto Call, typename Handle, typename Parent>
constexpr void
delete_dispatch(Handle handle, Parent parent, utility::detail::none)
{
  if (parent != nullptr)
  {
    Call(parent, handle, nullptr);
  }
}

template <auto Call, typename Handle, typename Parent, typename Pool>
constexpr void
delete_dispatch(Handle const & handle, Parent parent, Pool pool)
{
  if (parent != nullptr && pool != nullptr)
  {
    auto const [data, size] = utility::bind_data_and_size(handle);
    Call(parent, pool, static_cast<uint32_t>(size), data);
  }
}

} // namespace mvk::types::detail

#endif
