#ifndef MVK_TYPES_WRAPPER_HPP_INCLUDED
#define MVK_TYPES_WRAPPER_HPP_INCLUDED

#include "types/detail/deleter.hpp"
#include "utility/detail/exists.hpp"
#include "utility/detail/find_if.hpp"
#include "utility/detail/pack.hpp"
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
  constexpr explicit wrapper_handle_base(U && handle)
      : handle_(std::forward<U>(handle))
  {
  }

  [[nodiscard]] constexpr Handle const &
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
public:
  [[nodiscard]] static constexpr utility::detail::none
  get() noexcept
  {
    return {};
  }
};

template <typename Parent>
class wrapper_parent_base
{
public:
  using parent_type = Parent;

  constexpr wrapper_parent_base() noexcept = default;

  template <typename U>
  requires utility::not_this<U, wrapper_parent_base>
  constexpr explicit wrapper_parent_base(U && parent)
      : parent_(std::forward<U>(parent))
  {
  }

  [[nodiscard]] constexpr Parent const &
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
public:
  [[nodiscard]] static constexpr utility::detail::none
  parent() noexcept
  {
    return {};
  }
};

template <typename Pool>
class wrapper_pool_base
{
public:
  using pool_type = Pool;

  constexpr wrapper_pool_base() noexcept = default;

  template <typename U>
  requires utility::not_this<U, wrapper_pool_base>
  constexpr explicit wrapper_pool_base(U && pool)
      : pool_(std::forward<U>(pool))
  {
  }

  [[nodiscard]] constexpr Pool const &
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
public:
  [[nodiscard]] static constexpr utility::detail::none
  pool() noexcept
  {
    return {};
  }
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

  template <typename Handle, typename Parent, typename Pool>
  constexpr wrapper(Handle && handle, Parent && parent, Pool && pool) noexcept
      : handle_base(std::forward<Handle>(handle)),
        parent_base(std::forward<Parent>(parent)),
        pool_base(std::forward<Pool>(pool))
  {
  }

  template <typename Handle, typename Parent>
  constexpr wrapper(Handle && handle, Parent && parent) noexcept
      : handle_base(std::forward<Handle>(handle)),
        parent_base(std::forward<Parent>(parent))
  {
  }

  template <typename Handle>
  requires utility::not_this<Handle, wrapper>
  constexpr explicit wrapper(Handle && handle) noexcept
      : handle_base(std::forward<Handle>(handle))
  {
  }

  wrapper(wrapper const & other) noexcept = delete;
  wrapper(wrapper && other) noexcept
  {
    if constexpr (!decltype(utility::detail::is_none(handle_base::get())){})
    {
      std::swap(handle_base::get(), other.get());
    }

    if constexpr (!decltype(utility::detail::is_none(
                      parent_base::parent())){})
    {
      std::swap(parent_base::parent(), other.parent());
    }

    if constexpr (!decltype(utility::detail::is_none(pool_base::pool())){})
    {
      std::swap(pool_base::pool(), other.pool());
    }
  }

  wrapper &
  operator=(wrapper const & other) noexcept = delete;

  wrapper &
  operator=(wrapper && other) noexcept
  {
    if constexpr (!decltype(utility::detail::is_none(handle_base::get())){})
    {
      std::swap(handle_base::get(), other.get());
    }

    if constexpr (!decltype(utility::detail::is_none(
                      parent_base::parent())){})
    {
      std::swap(parent_base::parent(), other.parent());
    }

    if constexpr (!decltype(utility::detail::is_none(pool_base::pool())){})
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
    auto const & handle = handle_base::get();
    auto const & parent = parent_base::parent();
    auto const & pool = pool_base::pool();
    delete_dispatch<call>(handle, parent, pool);
  }
};

} // namespace mvk::types::detail

#endif
