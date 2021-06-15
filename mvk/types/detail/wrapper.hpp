#ifndef MVK_TYPES_WRAPPER_HPP_INCLUDED
#define MVK_TYPES_WRAPPER_HPP_INCLUDED

#include "utility/detail/exists.hpp"
#include "utility/detail/find_if.hpp"
#include "utility/detail/pack.hpp"
#include "utility/misc.hpp"
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

template <auto Call>
struct deleter
{
};

template <typename Handle>
class wrapper_handle_base
{
public:
  using handle_type = Handle;

  constexpr wrapper_handle_base() noexcept = default;

  template <typename U>
  requires utility::not_this<U, wrapper_handle_base>
  constexpr explicit wrapper_handle_base(U && handle) noexcept
      : handle_(std::forward<U>(handle))
  {
  }

  [[nodiscard]] constexpr utility::trivially_t<Handle>
  get() const noexcept
  {
    return handle_;
  }

  [[nodiscard]] constexpr Handle &
  get() noexcept
  {
    return handle_;
  }

private:
  Handle handle_ = {};
};

template <>
class wrapper_handle_base<utility::detail::none>
{
};

template <typename Parent>
class wrapper_parent_base
{
public:
  using parent_type = Parent;

  constexpr wrapper_parent_base() noexcept = default;

  template <typename U>
  requires utility::not_this<U, wrapper_parent_base>
  constexpr explicit wrapper_parent_base(U && parent) noexcept
      : parent_(std::forward<U>(parent))
  {
  }

  [[nodiscard]] constexpr utility::trivially_t<Parent>
  parent() const noexcept
  {
    return parent_;
  }

  [[nodiscard]] constexpr Parent &
  parent() noexcept
  {
    return parent_;
  }

private:
  Parent parent_ = {};
};

template <>
class wrapper_parent_base<utility::detail::none>
{
};

template <typename Pool>
class wrapper_pool_base
{
public:
  using pool_type = Pool;

  constexpr wrapper_pool_base() noexcept = default;

  template <typename U>
  requires utility::not_this<U, wrapper_pool_base>
  constexpr explicit wrapper_pool_base(U && pool) noexcept
      : pool_(std::forward<U>(pool))
  {
  }

  [[nodiscard]] constexpr utility::trivially_t<Pool>
  pool() const noexcept
  {
    return pool_;
  }

  [[nodiscard]] constexpr Pool &
  pool() noexcept
  {
    return pool_;
  }

private:
  Pool pool_ = {};
};

template <>
class wrapper_pool_base<utility::detail::none>
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

template <typename... Arguments>
class wrapper
    : public wrapper_handle_base<decltype(utility::detail::unpack_tag(
          utility::detail::find_if(utility::detail::pack<Arguments...>{},
                                   utility::detail::tagged_with<handle>())))>,
      public wrapper_parent_base<decltype(utility::detail::unpack_tag(
          utility::detail::find_if(utility::detail::pack<Arguments...>{},
                                   utility::detail::tagged_with<parent>())))>,
      public wrapper_pool_base<decltype(utility::detail::unpack_tag(
          utility::detail::find_if(utility::detail::pack<Arguments...>{},
                                   utility::detail::tagged_with<pool>())))>
{

  using handle_type = decltype(utility::detail::unpack_tag(
      utility::detail::find_if(utility::detail::pack<Arguments...>{},
                               utility::detail::tagged_with<handle>())));

  using handle_base = wrapper_handle_base<handle_type>;
  using parent_base =
      wrapper_parent_base<decltype(utility::detail::unpack_tag(
          utility::detail::find_if(utility::detail::pack<Arguments...>{},
                                   utility::detail::tagged_with<parent>())))>;

  using pool_base = wrapper_pool_base<decltype(utility::detail::unpack_tag(
      utility::detail::find_if(utility::detail::pack<Arguments...>{},
                               utility::detail::tagged_with<pool>())))>;

  static constexpr auto call = utility::detail::unpack_tag(
      utility::detail::find_if(utility::detail::pack<Arguments...>{},
                               utility::detail::tagged_with<deleter>()));

public:
  constexpr wrapper() noexcept = default;

  template <typename Handle, typename Parent, typename Pool,
            typename Wrapper = wrapper>
  requires has_get<Wrapper> && has_parent<Wrapper> && has_pool<Wrapper>
  constexpr wrapper(Handle && handle, Parent && parent, Pool && pool) noexcept
      : handle_base(std::forward<Handle>(handle)),
        parent_base(std::forward<Parent>(parent)),
        pool_base(std::forward<Pool>(pool))
  {
  }

  template <typename Handle, typename Parent, typename Wrapper = wrapper>
  requires has_get<Wrapper> && has_parent<Wrapper> &&
      inverse<has_pool<Wrapper>>
  constexpr wrapper(Handle && handle, Parent && parent) noexcept
      : handle_base(std::forward<Handle>(handle)),
        parent_base(std::forward<Parent>(parent))
  {
  }

  template <typename Handle, typename Wrapper = wrapper>
  requires utility::not_this<Handle, wrapper> && has_get<Wrapper> &&
      inverse<has_parent<Wrapper>> && inverse<has_pool<Wrapper>>
  constexpr explicit wrapper(Handle && handle) noexcept
      : handle_base(std::forward<Handle>(handle))
  {
  }

  wrapper(wrapper const & other) noexcept = delete;
  wrapper(wrapper && other) noexcept
  {
    if constexpr (has_get<wrapper>)
    {
      std::swap(handle_base::get(), other.get());
    }

    if constexpr (has_parent<wrapper>)
    {
      std::swap(parent_base::parent(), other.parent());
    }

    if constexpr (has_pool<wrapper>)
    {
      std::swap(pool_base::pool(), other.pool());
    }
  }

  wrapper &
  operator=(wrapper const & other) noexcept = delete;

  wrapper &
  operator=(wrapper && other) noexcept
  {
    if constexpr (has_get<wrapper>)
    {
      std::swap(handle_base::get(), other.get());
    }

    if constexpr (has_parent<wrapper>)
    {
      std::swap(parent_base::parent(), other.parent());
    }

    if constexpr (has_pool<wrapper>)
    {
      std::swap(pool_base::pool(), other.pool());
    }

    return *this;
  }

  constexpr void
  release() noexcept
  {
    *this = wrapper();
  }

  ~wrapper() noexcept
  {
    destroy();
  }

private:
  constexpr void
  destroy() const noexcept
  {
    if constexpr (utility::detail::is_none(call))
    {
      return;
    }
    else if constexpr (has_get<wrapper> && inverse<has_parent<wrapper>> &&
                       inverse<has_pool<wrapper>>)
    {
      call(handle_base::get(), nullptr);
    }
    else if constexpr (has_get<wrapper> && has_parent<wrapper> &&
                       inverse<has_pool<wrapper>>)
    {
      if (auto parent = parent_base::parent(); parent != nullptr)
      {
        call(parent_base::parent(), handle_base::get(), nullptr);
      }
    }
    else if constexpr (has_get<wrapper> && has_parent<wrapper> &&
                       has_pool<wrapper>)
    {
      auto parent = parent_base::parent();
      auto pool = pool_base::pool();
      if (parent != nullptr && pool != nullptr)
      {
        auto [data, size] = utility::bind_data_and_size(handle_base::get());
        call(parent, pool, static_cast<uint32_t>(size), data);
      }
    }
  }
};

} // namespace mvk::types::detail

#endif
