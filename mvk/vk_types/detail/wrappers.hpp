#ifndef MVK_VK_TYPES_WRAPPER_HPP_INCLUDED
#define MVK_VK_TYPES_WRAPPER_HPP_INCLUDED

#include "utility/compressed_pair.hpp"
#include "utility/verify.hpp"
#include "vk_types/detail/deleter.hpp"

#include <vector>

namespace mvk::vk_types::detail
{

template <typename Handle, typename Parent, typename Pool, auto DeleterCall>
class unique_wrapper_with_parent_and_pool_allocated
{
    using deleter_type = deleter<DeleterCall>;

    static_assert(std::is_same_v<Handle, typename deleter_type::handle_type>);
    static_assert(std::is_same_v<Parent, typename deleter_type::parent_type>);
    static_assert(std::is_same_v<Pool, typename deleter_type::pool_type>);

public:
    constexpr unique_wrapper_with_parent_and_pool_allocated() noexcept = default;

    unique_wrapper_with_parent_and_pool_allocated(std::vector<Handle> handles, Parent parent, Pool pool)
        : container_(utility::first_tag(), std::move(handles)), parent_(parent), pool_(pool)
    {
    }

    unique_wrapper_with_parent_and_pool_allocated(unique_wrapper_with_parent_and_pool_allocated const & other) noexcept = delete;

    unique_wrapper_with_parent_and_pool_allocated(unique_wrapper_with_parent_and_pool_allocated && other) noexcept
    {
        container_.swap(other.container_);
        std::swap(parent_, other.parent_);
        std::swap(pool_, other.pool_);
    }

    unique_wrapper_with_parent_and_pool_allocated &
    operator=(unique_wrapper_with_parent_and_pool_allocated const & other) noexcept = delete;

    unique_wrapper_with_parent_and_pool_allocated &
    operator=(unique_wrapper_with_parent_and_pool_allocated && other) noexcept
    {
        container_.swap(other.container_);
        std::swap(parent_, other.parent_);
        std::swap(pool_, other.pool_);
        return *this;
    }

    [[nodiscard]] std::vector<Handle> const &
    get() const noexcept
    {
        return container_.first();
    }

    [[nodiscard]] Parent
    parent() const noexcept
    {
        return parent_;
    }

    [[nodiscard]] Pool
    pool() const noexcept
    {
        return pool_;
    }

    void
    release() noexcept
    {
        if (parent_ != nullptr && pool_ != nullptr)
        {
            deleter()(parent_, pool_, get());

            parent_ = nullptr;
            pool_   = nullptr;
            reference().clear();
        }
    }

    ~unique_wrapper_with_parent_and_pool_allocated() noexcept
    {
        release();
    }

protected:
    [[nodiscard]] std::vector<Handle> &
    reference() noexcept
    {
        return container_.first();
    }

    [[nodiscard]] deleter_type const &
    deleter() noexcept
    {
        return container_.second();
    }

private:
    utility::compressed_pair<std::vector<Handle>, deleter_type> container_;
    Parent                                                      parent_ = nullptr;
    Pool                                                        pool_   = nullptr;
};

template <typename Handle, typename Parent, auto DeleterCall>
class unique_wrapper_with_parent
{
    using deleter_type = deleter<DeleterCall>;

    static_assert(std::is_same_v<Handle, typename deleter_type::handle_type>);
    static_assert(std::is_same_v<Parent, typename deleter_type::parent_type>);

public:
    constexpr unique_wrapper_with_parent() noexcept = default;

    constexpr unique_wrapper_with_parent(Handle handle, Parent parent) noexcept : container_(utility::first_tag(), handle), parent_(parent)
    {
    }

    unique_wrapper_with_parent(unique_wrapper_with_parent const & other) noexcept = delete;
    unique_wrapper_with_parent(unique_wrapper_with_parent && other) noexcept
    {
        container_.swap(other.container_);
        std::swap(parent_, other.parent_);
    }

    unique_wrapper_with_parent &
    operator=(unique_wrapper_with_parent const & other) noexcept = delete;
    unique_wrapper_with_parent &
    operator=(unique_wrapper_with_parent && other) noexcept
    {
        container_.swap(other.container_);
        std::swap(parent_, other.parent_);
        return *this;
    }

    [[nodiscard]] Handle
    get() const noexcept
    {
        return container_.first();
    }

    [[nodiscard]] Parent
    parent() const noexcept
    {
        return parent_;
    }

    void
    release() noexcept
    {
        if (parent_ != nullptr)
        {
            deleter()(parent_, get());

            parent_     = nullptr;
            reference() = nullptr;
        }
    }

    ~unique_wrapper_with_parent() noexcept
    {
        release();
    }

protected:
    [[nodiscard]] Handle &
    reference() noexcept
    {
        return container_.first();
    }

    [[nodiscard]] deleter_type const &
    deleter() noexcept
    {
        return container_.second();
    }

private:
    utility::compressed_pair<Handle, deleter_type> container_;
    Parent                                         parent_ = nullptr;
};

template <typename Handle, auto DeleterCall>
class unique_wrapper
{
    using deleter_type = deleter<DeleterCall>;
    static_assert(std::is_same_v<Handle, typename deleter_type::handle_type>);

public:
    constexpr unique_wrapper() noexcept = default;

    explicit unique_wrapper(Handle handle) noexcept : container_(utility::first_tag(), handle)
    {
    }

    unique_wrapper(unique_wrapper const & other) noexcept = delete;
    unique_wrapper(unique_wrapper && other) noexcept
    {
        container_.swap(other.container_);
    }

    unique_wrapper &
    operator=(unique_wrapper const & other) noexcept = delete;
    unique_wrapper &
    operator=(unique_wrapper && other) noexcept
    {
        container_.swap(other.container_);
        return *this;
    }

    [[nodiscard]] Handle
    get() const noexcept
    {
        return container_.first();
    }

    void
    release() noexcept
    {
        deleter()(get());
        reference() = nullptr;
    }

    ~unique_wrapper() noexcept
    {
        release();
    }

protected:
    [[nodiscard]] Handle &
    reference() noexcept
    {
        return container_.first();
    }

    [[nodiscard]] deleter_type const &
    deleter() noexcept
    {
        return container_.second();
    }

private:
    utility::compressed_pair<Handle, deleter_type> container_;
};

} // namespace mvk::vk_types::detail

#endif
