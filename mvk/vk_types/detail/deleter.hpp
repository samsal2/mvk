#ifndef MVK_VK_TYPE_DETAIL_DELETER_HPP_INCLUDED
#define MVK_VK_TYPE_DETAIL_DELETER_HPP_INCLUDED

#include "utility/misc.hpp"
#include "vk_types/common.hpp"
#include "vk_types/detail/deleter_base.hpp"
#include "vk_types/detail/get_container.hpp"

namespace mvk::vk_types::detail
{

template <auto DeleterCall, typename = decltype(DeleterCall)>
struct deleter;

template <auto DeleterCall, typename Instance>
struct deleter<DeleterCall, void (*)(Instance, VkAllocationCallbacks const *)> : public deleter_base<Instance>
{
        using deleter_base = deleter_base<Instance>;
        using deleter_base::deleter_base;

        static constexpr auto is_allocation = deleter_base::is_allocation::value;
        using container_type                = detail::get_container_t<Instance, is_allocation>;

        void
        operator()(container_type const instance) const noexcept
        {
                DeleterCall(instance, nullptr);
        }
};

template <auto DeleterCall, typename Parent, typename Instance>
struct deleter<DeleterCall, void (*)(Parent, Instance, VkAllocationCallbacks const *)> : public deleter_base<Instance, Parent>
{
        using deleter_base = deleter_base<Instance, Parent>;
        using deleter_base::deleter_base;

        static constexpr auto is_allocation = deleter_base::is_allocation::value;
        using container_type                = detail::get_container_t<Instance, is_allocation>;

        void
        operator()(container_type const instance) const noexcept
        {
                auto const parent = deleter_base::parent();
                if (parent != nullptr) [[likely]]
                {
                        DeleterCall(parent, instance, nullptr);
                }
        }
};

template <auto DeleterCall, typename Parent, typename Pool, typename Instance>
struct deleter<DeleterCall, void (*)(Parent, Pool, uint32_t, Instance const *)> : public deleter_base<Instance, Parent, Pool>
{
public:
        using deleter_base = deleter_base<Instance, Parent, Pool>;
        using deleter_base::deleter_base;

        static constexpr auto is_allocation = deleter_base::is_allocation::value;
        using container_type                = detail::get_container_t<Instance, is_allocation>;

        void
        operator()(container_type const & instance_source) const noexcept
        {
                auto const parent = deleter_base::parent();
                auto const pool   = deleter_base::pool();
                if (parent != nullptr && pool != nullptr) [[likely]]
                {
                        auto const [data, size] = utility::bind_data_and_size(instance_source);
                        DeleterCall(parent, pool, static_cast<uint32_t>(size), data);
                }
        }
};

template <auto DeleterCall, typename Parent, typename Pool, typename Instance>
struct deleter<DeleterCall, VkResult (*)(Parent, Pool, uint32_t, Instance const *)> : public deleter_base<Instance, Parent, Pool>
{
        using deleter_base = deleter_base<Instance, Parent, Pool>;
        using deleter_base::deleter_base;

        static constexpr auto is_allocation = deleter_base::is_allocation::value;
        using container_type                = detail::get_container_t<Instance, is_allocation>;

        void
        operator()(container_type const & instance_source) const noexcept
        {
                auto const parent = deleter_base::parent();
                auto const pool   = deleter_base::pool();
                if (parent != nullptr && pool != nullptr) [[likely]]
                {
                        auto const [data, size]            = utility::bind_data_and_size(instance_source);
                        [[maybe_unused]] auto const result = DeleterCall(parent, pool, static_cast<uint32_t>(size), data);
                }
        }
};

template <typename Deleter>
concept with_parent = requires(Deleter deleter)
{
        {deleter.parent()};
};

template <typename Deleter>
concept with_pool = requires(Deleter deleter)
{
        {deleter.pool()};
};

template <typename Deleter>
concept with_parent_and_pool = requires
{
        {with_parent<Deleter>};
        {with_pool<Deleter>};
};

} // namespace mvk::vk_types::detail

#endif
