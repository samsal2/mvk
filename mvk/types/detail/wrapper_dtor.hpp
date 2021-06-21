#ifndef MVK_TYPES_DETAIL_WRAPPER_DTOR_HANDLER
#define MVK_TYPES_DETAIL_WRAPPER_DTOR_HANDLER

#include "types/common.hpp"
#include "utility/misc.hpp"
#include "utility/slice.hpp"

namespace mvk::types::detail
{

template <auto Call, typename = decltype(Call)>
struct wrapper_dtor;

template <auto Call, typename Handle>
struct wrapper_dtor<Call, void (*)(Handle, VkAllocationCallbacks const *)>
{
  static constexpr void
  destroy(Handle const handle) noexcept
  {
    Call(handle, nullptr);
  }
};

template <auto Call, typename Handle, typename Parent>
struct wrapper_dtor<Call,
                    void (*)(Parent, Handle, VkAllocationCallbacks const *)>
{
  static constexpr void
  destroy(Parent const parent, Handle const handle) noexcept
  {
    if (parent != VK_NULL_HANDLE)
    {
      Call(parent, handle, nullptr);
    }
  }
};

template <auto Call, typename Handle, typename Parent, typename Pool>
struct wrapper_dtor<Call, void (*)(Parent, Pool, uint32_t, Handle const *)>

{
  static constexpr void
  destroy(Parent const parent, Pool const pool,
          utility::slice<Handle> handles) noexcept
  {
    if (parent != VK_NULL_HANDLE && pool != VK_NULL_HANDLE)
    {
      auto [data, size] = utility::bind_data_and_size(handles);
      Call(parent, pool, static_cast<uint32_t>(size), data);
    }
  }
};

template <auto Call, typename Handle, typename Parent, typename Pool>
struct wrapper_dtor<Call,
                    VkResult (*)(Parent, Pool, uint32_t, Handle const *)>

{
  static constexpr void
  destroy(Parent const parent, Pool const pool,
          utility::slice<Handle> handles) noexcept
  {
    if (parent != VK_NULL_HANDLE && pool != VK_NULL_HANDLE)
    {
      auto [data, size] = utility::bind_data_and_size(handles);
      Call(parent, pool, static_cast<uint32_t>(size), data);
    }
  }
};

}; // namespace mvk::types::detail

#endif
