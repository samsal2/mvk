#ifndef MVK_TYPES_WRAPPER_HPP_INCLUDED
#define MVK_TYPES_WRAPPER_HPP_INCLUDED

#include "types/common.hpp"
#include "types/validation/validation.hpp"
#include "utility/detail/exists.hpp"
#include "utility/detail/find_if.hpp"
#include "utility/detail/pack.hpp"
#include "utility/misc.hpp"
#include "utility/verify.hpp"

#include <vector>

namespace mvk::types::detail
{

// Define wrapper tags
template <typename Handle> struct handle
{
};

template <typename Parent> struct parent
{
};

template <typename Pool> struct pool
{
};

template <auto Call> struct deleter
{
};

template <auto Call> struct creator
{
};

template <typename Handle> class wrapper_handle_base
{
public:
        using handle_type = Handle;

        constexpr wrapper_handle_base() noexcept = default;

        template <typename U>
        requires utility::not_this<U, wrapper_handle_base>
        constexpr explicit wrapper_handle_base(U && handle) noexcept : handle_(std::forward<U>(handle))
        {
        }

        [[nodiscard]] constexpr Handle const & get() const noexcept
        {
                return handle_;
        }

        [[nodiscard]] constexpr Handle & get() noexcept
        {
                return handle_;
        }

private:
        Handle handle_ = {};
};

template <> class wrapper_handle_base<utility::detail::none>
{
};

template <typename Parent> class wrapper_parent_base
{
public:
        using parent_type = Parent;

        constexpr wrapper_parent_base() noexcept = default;

        template <typename U>
        requires utility::not_this<U, wrapper_parent_base>
        constexpr explicit wrapper_parent_base(U && parent) noexcept : parent_(std::forward<U>(parent))
        {
        }

        [[nodiscard]] constexpr Parent const & parent() const noexcept
        {
                return parent_;
        }

        [[nodiscard]] constexpr Parent & parent() noexcept
        {
                return parent_;
        }

private:
        Parent parent_ = {};
};

template <> class wrapper_parent_base<utility::detail::none>
{
};

template <typename Pool> class wrapper_pool_base
{
public:
        using pool_type = Pool;

        constexpr wrapper_pool_base() noexcept = default;

        template <typename U>
        requires utility::not_this<U, wrapper_pool_base>
        constexpr explicit wrapper_pool_base(U && pool) noexcept : pool_(std::forward<U>(pool))
        {
        }

        [[nodiscard]] constexpr Pool const & pool() const noexcept
        {
                return pool_;
        }

        [[nodiscard]] constexpr Pool & pool() noexcept
        {
                return pool_;
        }

private:
        Pool pool_ = {};
};

template <> class wrapper_pool_base<utility::detail::none>
{
};

template <typename Wrapper>
concept has_get = requires(Wrapper wrapper)
{
        {wrapper.get()};
};

template <typename Wrapper>
concept has_pool = requires(Wrapper wrapper)
{
        {wrapper.pool()};
};

template <typename Wrapper>
concept has_parent = requires(Wrapper wrapper)
{
        {wrapper.parent()};
};

// Doing requires (!concept) messes up clang-format
template <bool B>
concept inverse = requires
{
        requires !B;
};

template <auto Call, typename = decltype(Call)> struct find_info_type_impl;
template <auto Call, typename Handle, typename Info> struct find_info_type_impl<Call, VkResult (*)(Info const *, VkAllocationCallbacks const *, Handle *)>
{
        using type = Info;
};

template <auto Call, typename Handle, typename Parent, typename Info> struct find_info_type_impl<Call, VkResult (*)(Parent, Info const *, VkAllocationCallbacks const *, Handle *)>
{
        using type = Info;
};

template <auto Call, typename Handle, typename Parent, typename Info, typename Result> struct find_info_type_impl<Call, Result (*)(Parent, Info const *, Handle *)>
{
        using type = Info;
};

template <auto Call> using find_info_t = typename find_info_type_impl<Call>::type;

constexpr uint32_t size_from_info(VkCommandBufferAllocateInfo const & info)
{
        return info.commandBufferCount;
}
constexpr VkCommandPool pool_from_info(VkCommandBufferAllocateInfo const & info)
{
        return info.commandPool;
}

constexpr uint32_t size_from_info(VkDescriptorSetAllocateInfo const & info)
{
        return info.descriptorSetCount;
}

constexpr VkDescriptorPool pool_from_info(VkDescriptorSetAllocateInfo const & info)
{
        return info.descriptorPool;
}

template <auto Call, typename Handle, typename Parent, typename Pool> struct creator_handler
{
        template <typename ParentReference> static constexpr Handle create(ParentReference && parent, find_info_t<Call> const & info)
        {
                auto handle = Handle(size_from_info(info));
                Call(std::forward<ParentReference>(parent), &info, std::data(handle));
                return handle;
        }
};

template <auto Call, typename Handle> struct creator_handler<Call, Handle, utility::detail::none, utility::detail::none>
{
        static constexpr Handle create(find_info_t<Call> const & info)
        {
                auto handle = Handle();
                Call(&info, nullptr, &handle);
                return handle;
        }
};

template <auto Call, typename Handle, typename Parent> struct creator_handler<Call, Handle, Parent, utility::detail::none>
{
        template <typename ParentReference>
        requires utility::same_as<std::decay_t<ParentReference>, Parent>
        static constexpr Handle create(ParentReference && parent, find_info_t<Call> const & info)
        {
                auto handle = Handle();
                Call(std::forward<ParentReference>(parent), &info, nullptr, &handle);
                return handle;
        }
};

template <> struct creator_handler<vkCreateDevice, VkDevice, utility::detail::none, utility::detail::none>
{
        static constexpr VkDevice create(VkPhysicalDevice const parent, find_info_t<vkCreateDevice> const & info)
        {
                auto handle = VkDevice();
                vkCreateDevice(parent, &info, nullptr, &handle);
                return handle;
        }
};

template <typename Handle> struct creator_handler<utility::detail::none{}, Handle, utility::detail::none, utility::detail::none>
{
        template <typename HandleReference> static constexpr decltype(auto) create(HandleReference && handle)
        {
                return std::forward<HandleReference>(handle);
        }
};

template <> struct creator_handler<vkGetDeviceQueue, VkQueue, utility::detail::none, utility::detail::none>
{

        [[nodiscard]] static constexpr VkQueue create(VkDevice const device, uint32_t const index) noexcept
        {
                auto handle = VkQueue();
                vkGetDeviceQueue(device, index, 0, &handle);
                return handle;
        }
};

template <> struct creator_handler<vkCreateGraphicsPipelines, VkPipeline, VkDevice, utility::detail::none>
{

        [[nodiscard]] static constexpr VkPipeline create(VkDevice const device, VkGraphicsPipelineCreateInfo const & info) noexcept
        {
                auto handle = VkPipeline();
                vkCreateGraphicsPipelines(device, nullptr, 1, &info, nullptr, &handle);
                return handle;
        }
};

template <> struct creator_handler<validation::setup_debug_messenger, VkDebugUtilsMessengerEXT, VkInstance, utility::detail::none>
{
        [[nodiscard]] static constexpr auto create(VkInstance const instance)
        {
                return validation::setup_debug_messenger(instance);
        }
};

template <> struct creator_handler<glfwCreateWindowSurface, VkSurfaceKHR, VkInstance, utility::detail::none>
{

        [[nodiscard]] static constexpr VkSurfaceKHR create(VkInstance const instance, GLFWwindow * window) noexcept
        {
                auto handle = VkSurfaceKHR();
                glfwCreateWindowSurface(instance, window, nullptr, &handle);
                return handle;
        }
};

template <typename... Arguments>
class wrapper_members_base : public wrapper_handle_base<decltype(utility::detail::unpack_tag(utility::detail::find_if(utility::detail::pack<Arguments...>{}, utility::detail::tagged_with<handle>())))>,
                             public wrapper_parent_base<decltype(utility::detail::unpack_tag(utility::detail::find_if(utility::detail::pack<Arguments...>{}, utility::detail::tagged_with<parent>())))>,
                             public wrapper_pool_base<decltype(utility::detail::unpack_tag(utility::detail::find_if(utility::detail::pack<Arguments...>{}, utility::detail::tagged_with<pool>())))>
{
protected:
        using handle_type = decltype(utility::detail::unpack_tag(utility::detail::find_if(utility::detail::pack<Arguments...>{}, utility::detail::tagged_with<handle>())));
        using parent_type = decltype(utility::detail::unpack_tag(utility::detail::find_if(utility::detail::pack<Arguments...>{}, utility::detail::tagged_with<parent>())));
        using pool_type   = decltype(utility::detail::unpack_tag(utility::detail::find_if(utility::detail::pack<Arguments...>{}, utility::detail::tagged_with<pool>())));
};

template <auto Deleter, typename... Arguments> class wrapper_deleter_base : public wrapper_members_base<Arguments...>
{
private:
        using base                         = wrapper_members_base<Arguments...>;
        static constexpr auto deleter_call = Deleter;

public:
        constexpr wrapper_deleter_base() noexcept = default;

        wrapper_deleter_base(wrapper_deleter_base const & other) noexcept = delete;
        wrapper_deleter_base(wrapper_deleter_base && other) noexcept
        {
                if constexpr (has_get<base>)
                {
                        std::swap(base::get(), other.get());
                }

                if constexpr (has_parent<base>)
                {
                        std::swap(base::parent(), other.parent());
                }

                if constexpr (has_pool<base>)
                {
                        std::swap(base::pool(), other.pool());
                }
        }

        wrapper_deleter_base & operator=(wrapper_deleter_base const & other) noexcept = delete;

        wrapper_deleter_base & operator=(wrapper_deleter_base && other) noexcept
        {
                if constexpr (has_get<base>)
                {
                        std::swap(base::get(), other.get());
                }

                if constexpr (has_parent<base>)
                {
                        std::swap(base::parent(), other.parent());
                }

                if constexpr (has_pool<base>)
                {
                        std::swap(base::pool(), other.pool());
                }

                return *this;
        }

        constexpr void release() noexcept
        {
                *this = wrapper_deleter_base();
        }

        ~wrapper_deleter_base() noexcept
        {
                destroy();
        }

private:
        constexpr void destroy() const noexcept
        {
                if constexpr (has_get<base> && inverse<has_parent<base>> && inverse<has_pool<base>>)
                {
                        deleter_call(base::get(), nullptr);
                }
                else if constexpr (has_get<base> && has_parent<base> && inverse<has_pool<base>>)
                {
                        if (auto parent = base::parent(); parent != nullptr)
                        {
                                deleter_call(base::parent(), base::get(), nullptr);
                        }
                }
                else if constexpr (has_get<base> && has_parent<base> && has_pool<base>)
                {
                        auto parent = base::parent();
                        auto pool   = base::pool();
                        if (parent != nullptr && pool != nullptr)
                        {
                                auto [data, size] = utility::bind_data_and_size(base::get());
                                deleter_call(parent, pool, static_cast<uint32_t>(size), data);
                        }
                }
        }
};

template <typename... Arguments> class wrapper_deleter_base<utility::detail::none{}, Arguments...> : public wrapper_members_base<Arguments...>
{
};

template <auto Creator, typename... Arguments> class wrapper_creator_base : public wrapper_deleter_base<utility::detail::unpack_tag(utility::detail::find_if(utility::detail::pack<Arguments...>{}, utility::detail::tagged_with<deleter>())), Arguments...>
{
private:
        using base = wrapper_deleter_base<utility::detail::unpack_tag(utility::detail::find_if(utility::detail::pack<Arguments...>{}, utility::detail::tagged_with<deleter>())), Arguments...>;

        static constexpr auto creator_call = Creator;

public:
        constexpr explicit wrapper_creator_base() noexcept = default;

        template <typename U, typename... Us>
        requires utility::not_this<U, wrapper_creator_base>
        constexpr explicit(!std::is_trivial_v<typename base::handle_type>) wrapper_creator_base(U && arg, Us &&... args) noexcept
        {
                if constexpr (has_parent<base>)
                {
                        auto parent = utility::detail::find_if(utility::detail::same_as<typename base::parent_type>(), std::forward<U>(arg), std::forward<Us>(args)...);
                        static_assert(!decltype(utility::detail::is_none(parent)){});
                        base::parent() = parent;
                }

                if constexpr (has_pool<base>)
                {
                        auto info    = utility::detail::find_if(utility::detail::same_as<find_info_t<creator_call>>(), std::forward<U>(arg), std::forward<Us>(args)...);
                        base::pool() = pool_from_info(info);
                }

                if constexpr (has_get<base>)
                {
                        base::get() = creator_handler<creator_call, typename base::handle_type, typename base::parent_type, typename base::pool_type>::create(std::forward<U>(arg), std::forward<Us>(args)...);
                }
        }
};

template <typename... Arguments> class wrapper : public wrapper_creator_base<utility::detail::unpack_tag(utility::detail::find_if(utility::detail::pack<Arguments...>{}, utility::detail::tagged_with<creator>())), Arguments...>
{
private:
        using base = wrapper_creator_base<utility::detail::unpack_tag(utility::detail::find_if(utility::detail::pack<Arguments...>{}, utility::detail::tagged_with<creator>())), Arguments...>;

public:
        using base::base;
};

} // namespace mvk::types::detail

#endif
