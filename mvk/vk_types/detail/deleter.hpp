#ifndef MVK_VK_TYPE_DETAIL_DELETER_HPP_INCLUDED
#define MVK_VK_TYPE_DETAIL_DELETER_HPP_INCLUDED

#include "utility/misc.hpp"
#include "utility/slice.hpp"
#include "vk_types/common.hpp"

namespace mvk::vk_types::detail
{

template <auto DeleterCall, typename = decltype(DeleterCall)>
struct deleter;

template <auto DeleterCall, typename Handle>
struct deleter<DeleterCall, void (*)(Handle, VkAllocationCallbacks const *)>
{
    using handle_type = Handle;

    void
    operator()(Handle const handle) const noexcept
    {
        DeleterCall(handle, nullptr);
    }
};

template <auto DeleterCall, typename Parent, typename Handle>
struct deleter<DeleterCall, void (*)(Parent, Handle, VkAllocationCallbacks const *)>
{
    using handle_type = Handle;
    using parent_type = Parent;

    void
    operator()(Parent const parent, Handle const handle) const noexcept
    {
        DeleterCall(parent, handle, nullptr);
    }
};

template <auto DeleterCall, typename Parent, typename Pool, typename Handle>
struct deleter<DeleterCall, void (*)(Parent, Pool, uint32_t, Handle const *)>
{
    using handle_type = Handle;
    using parent_type = Parent;
    using pool_type   = Pool;

    void
    operator()(Parent const parent, Pool const pool, utility::slice<Handle> const handles) const noexcept
    {
        auto const [data, size] = utility::bind_data_and_size(handles);
        DeleterCall(parent, pool, static_cast<uint32_t>(size), data);
    }
};

template <auto DeleterCall, typename Parent, typename Pool, typename Handle>
struct deleter<DeleterCall, VkResult (*)(Parent, Pool, uint32_t, Handle const *)>
{
    using handle_type = Handle;
    using parent_type = Parent;
    using pool_type   = Pool;

    void
    operator()(Parent const parent, Pool const pool, utility::slice<Handle> const handles) const noexcept
    {
        auto const [data, size]      = utility::bind_data_and_size(handles);
        [[maybe_unused]] auto result = DeleterCall(parent, pool, static_cast<uint32_t>(size), data);
    }
};

} // namespace mvk::vk_types::detail

#endif
