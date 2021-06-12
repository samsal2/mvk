#ifndef MVK_TYPES_WRAPPER_HPP_INCLUDED
#define MVK_TYPES_WRAPPER_HPP_INCLUDED

#include "glm/packing.hpp"
#include "types/detail/deleter.hpp"
#include "utility/verify.hpp"

#include <vector>

namespace mvk::types::detail
{

// Define wrapper tags
template <typename Handle>
struct handle
{
};

template <typename Parent>
struct parent
{
};

template <typename Pool>
struct pool
{
};

template <typename... Arguments>
class wrapper;

template <auto DeleterCall, typename Handle>
class wrapper<deleter<DeleterCall>, handle<Handle>>
{
    using deleter_type = deleter<DeleterCall>;

public:
    constexpr wrapper() noexcept = default;

    explicit wrapper(Handle handle) noexcept : handle_(handle)
    {
    }

    wrapper(wrapper const & other) noexcept = delete;
    wrapper(wrapper && other) noexcept
    {
        std::swap(handle_, other.handle_);
    }

    wrapper &
    operator=(wrapper const & other) noexcept = delete;
    wrapper &
    operator=(wrapper && other) noexcept
    {
        std::swap(handle_, other.handle_);
        return *this;
    }

    [[nodiscard]] constexpr Handle
    get() const noexcept
    {
        return handle_;
    }

    constexpr void
    release() noexcept
    {
        destroy();
        reference() = nullptr;
    }

    ~wrapper() noexcept
    {
        destroy();
    }

protected:
    constexpr void
    destroy() noexcept
    {
        deleter()(get());
    }

    [[nodiscard]] constexpr Handle &
    reference() noexcept
    {
        return handle_;
    }

    [[nodiscard]] constexpr deleter_type
    deleter() noexcept
    {
        return {};
    }

private:
    Handle handle_ = nullptr;
};

template <auto DeleterCall, typename Handle, typename Parent>
class wrapper<deleter<DeleterCall>, handle<Handle>, parent<Parent>>
{
    using deleter_type = deleter<DeleterCall>;

public:
    constexpr wrapper() noexcept = default;

    constexpr wrapper(Handle handle, Parent parent) noexcept : handle_(handle), parent_(parent)
    {
    }

    wrapper(wrapper const & other) noexcept = delete;
    wrapper(wrapper && other) noexcept
    {
        std::swap(handle_, other.handle_);
        std::swap(parent_, other.parent_);
    }

    wrapper &
    operator=(wrapper const & other) noexcept = delete;
    wrapper &
    operator=(wrapper && other) noexcept
    {
        std::swap(handle_, other.handle_);
        std::swap(parent_, other.parent_);
        return *this;
    }

    [[nodiscard]] constexpr Handle
    get() const noexcept
    {
        return handle_;
    }

    [[nodiscard]] constexpr Parent
    parent() const noexcept
    {
        return parent_;
    }

    constexpr void
    release() noexcept
    {
        destroy();
        parent_     = nullptr;
        reference() = nullptr;
    }

    ~wrapper() noexcept
    {
        destroy();
    }

protected:
    constexpr void
    destroy() noexcept
    {
        if (parent_ != nullptr)
        {
            deleter()(parent_, get());
        }
    }

    [[nodiscard]] constexpr Handle &
    reference() noexcept
    {
        return handle_;
    }

    [[nodiscard]] constexpr deleter_type
    deleter() noexcept
    {
        return {};
    }

private:
    Handle handle_ = nullptr;
    Parent parent_ = nullptr;
};

template <auto DeleterCall, typename Handle, typename Parent, typename Pool>
class wrapper<deleter<DeleterCall>, handle<Handle>, parent<Parent>, pool<Pool>>
{
    using deleter_type = deleter<DeleterCall>;

public:
    constexpr wrapper() noexcept = default;

    wrapper(Handle handles, Parent parent, Pool pool) : handles_(std::move(handles)), parent_(parent), pool_(pool)
    {
    }

    wrapper(wrapper const & other) noexcept = delete;
    wrapper(wrapper && other) noexcept
    {
        std::swap(handles_, other.handles_);
        std::swap(parent_, other.parent_);
        std::swap(pool_, other.pool_);
    }

    wrapper &
    operator=(wrapper const & other) noexcept = delete;

    wrapper &
    operator=(wrapper && other) noexcept
    {
        std::swap(handles_, other.handles_);
        std::swap(parent_, other.parent_);
        std::swap(pool_, other.pool_);
        return *this;
    }

    [[nodiscard]] constexpr Handle const &
    get() const noexcept
    {
        return handles_;
    }

    [[nodiscard]] constexpr Parent
    parent() const noexcept
    {
        return parent_;
    }

    [[nodiscard]] constexpr Pool
    pool() const noexcept
    {
        return pool_;
    }

    constexpr void
    release() noexcept
    {
        destroy();
        parent_ = nullptr;
        pool_   = nullptr;
        reference().clear();
    }

    ~wrapper() noexcept
    {
        destroy();
    }

protected:
    constexpr void
    destroy()
    {
        if (parent_ != nullptr && pool_ != nullptr)
        {
            deleter()(parent_, pool_, get());
        }
    }

    [[nodiscard]] constexpr Handle &
    reference() noexcept
    {
        return handles_;
    }

    [[nodiscard]] constexpr deleter_type
    deleter() noexcept
    {
        return {};
    }

private:
    Handle handles_ = {};
    Parent parent_  = nullptr;
    Pool   pool_    = nullptr;
};

} // namespace mvk::types::detail

#endif
