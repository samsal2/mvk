#ifndef MVK_TYPES_WRAPPER_HPP_INCLUDED
#define MVK_TYPES_WRAPPER_HPP_INCLUDED

#include "types/detail/deleter.hpp"
#include "utility/meta/exists.hpp"
#include "utility/meta/find_if.hpp"
#include "utility/meta/pack.hpp"
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

  // clang-format off
  template <typename U>
  requires(
      utility::same_as<std::decay_t<U>, handle_type> ||
      utility::same_as<std::decay_t<U>, std::nullptr_t>) 
  constexpr explicit wrapper_handle_base(U && handle)
      : handle_(std::forward<U>(handle))
  {
  }
  // clang-format on

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
class wrapper_handle_base<utility::meta::none>
{
public:
  [[nodiscard]] constexpr utility::meta::none
  get() const noexcept
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
  requires utility::same_as<std::decay_t<U>, parent_type>
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
class wrapper_parent_base<utility::meta::none>
{
public:
  [[nodiscard]] constexpr utility::meta::none
  parent() const noexcept
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
  requires utility::same_as<std::decay_t<U>, pool_type>
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
class wrapper_pool_base<utility::meta::none>
{
public:
  [[nodiscard]] constexpr utility::meta::none
  pool() const noexcept
  {
    return {};
  }
};

template <typename T>
using uncvref_t = std::remove_cvref_t<T>;

template <typename... Arguments>
class wrapper
    : public wrapper_handle_base<decltype(utility::meta::unpack_tag(
          utility::meta::find_if(utility::meta::pack<Arguments...>{},
                                 utility::meta::tagged_with<handle>())))>,
      public wrapper_parent_base<decltype(utility::meta::unpack_tag(
          utility::meta::find_if(utility::meta::pack<Arguments...>{},
                                 utility::meta::tagged_with<parent>())))>,
      public wrapper_pool_base<decltype(utility::meta::unpack_tag(
          utility::meta::find_if(utility::meta::pack<Arguments...>{},
                                 utility::meta::tagged_with<pool>())))>
{

  using handle_type = decltype(utility::meta::unpack_tag(
      utility::meta::find_if(utility::meta::pack<Arguments...>{},
                             utility::meta::tagged_with<handle>())));

  using handle_base = wrapper_handle_base<handle_type>;
  using parent_base = wrapper_parent_base<decltype(utility::meta::unpack_tag(
      utility::meta::find_if(utility::meta::pack<Arguments...>{},
                             utility::meta::tagged_with<parent>())))>;

  using pool_base = wrapper_pool_base<decltype(utility::meta::unpack_tag(
      utility::meta::find_if(utility::meta::pack<Arguments...>{},
                             utility::meta::tagged_with<pool>())))>;

  static constexpr auto call = utility::meta::unpack_tag(
      utility::meta::find_if(utility::meta::pack<Arguments...>{},
                             utility::meta::tagged_with<deleter>()));

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
  requires utility::same_as<std::decay_t<Handle>, handle_type>
  constexpr explicit wrapper(Handle && handle) noexcept
      : handle_base(std::forward<Handle>(handle))
  {
  }

  wrapper(wrapper const & other) noexcept = delete;
  wrapper(wrapper && other) noexcept
  {
    utility::meta::avoid_none_swap(handle_base::get(), other.get());
    utility::meta::avoid_none_swap(parent_base::parent(), other.parent());
    utility::meta::avoid_none_swap(pool_base::pool(), other.pool());
  }

  wrapper &
  operator=(wrapper const & other) noexcept = delete;

  wrapper &
  operator=(wrapper && other) noexcept
  {
    utility::meta::avoid_none_swap(handle_base::get(), other.get());
    utility::meta::avoid_none_swap(parent_base::parent(), other.parent());
    utility::meta::avoid_none_swap(pool_base::pool(), other.pool());
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
    delete_dispatch<call>(handle_base::get(), parent_base::parent(),
                          pool_base::pool());
  }
};

} // namespace mvk::types::detail

#endif
