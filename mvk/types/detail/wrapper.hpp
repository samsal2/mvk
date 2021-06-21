#ifndef MVK_TYPES_WRAPPER_HPP_INCLUDED
#define MVK_TYPES_WRAPPER_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper_ctor.hpp"
#include "types/detail/wrapper_dtor.hpp"
#include "utility/misc.hpp"
#include "utility/pack.hpp"
#include "utility/verify.hpp"

#include <type_traits>
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

template <auto Call>
struct creator
{
};

template <typename... Args>
constexpr auto
find_handle([[maybe_unused]] utility::pack<Args...> elements) noexcept
{
  return utility::unpack_tag(
      utility::find_if(utility::tagged_with<handle>(), elements));
}

template <typename... Args>
constexpr auto
find_parent([[maybe_unused]] utility::pack<Args...> elements) noexcept
{
  return utility::unpack_tag(
      utility::find_if(utility::tagged_with<parent>(), elements));
}

template <typename... Args>
constexpr auto
find_pool([[maybe_unused]] utility::pack<Args...> elements) noexcept
{
  return utility::unpack_tag(
      utility::find_if(utility::tagged_with<pool>(), elements));
}

template <typename... Args>
constexpr auto
find_deleter([[maybe_unused]] utility::pack<Args...> elements) noexcept
{
  return utility::unpack_tag(
      utility::find_if(utility::tagged_with<deleter>(), elements));
}

template <typename... Args>
constexpr auto
find_creator([[maybe_unused]] utility::pack<Args...> elements) noexcept
{
  return utility::unpack_tag(
      utility::find_if(utility::tagged_with<creator>(), elements));
}

template <typename Handle>
class wrapper_handle_base
{
public:
  using handle_type = Handle;

  constexpr wrapper_handle_base() noexcept = default;

  constexpr handle_type &
  get() noexcept
  {
    return handle_;
  }

  constexpr handle_type const &
  get() const noexcept
  {
    return handle_;
  }

protected:
  template <typename U>
  requires utility::not_this<U, wrapper_handle_base>
  constexpr explicit wrapper_handle_base(U && handle)
      : handle_(std::forward<U>(handle))
  {
  }

private:
  Handle handle_ = {};
};

template <>
class wrapper_handle_base<utility::none>
{
};

template <typename Parent>
class wrapper_parent_base
{
public:
  using parent_type = Parent;

  constexpr wrapper_parent_base() noexcept = default;

  constexpr parent_type &
  parent() noexcept
  {
    return parent_;
  }

  constexpr parent_type const &
  parent() const noexcept
  {
    return parent_;
  }

protected:
  template <typename U>
  requires utility::not_this<U, wrapper_parent_base>
  constexpr explicit wrapper_parent_base(U && parent)
      : parent_(std::forward<U>(parent))
  {
  }

private:
  Parent parent_ = {};
};

template <>
class wrapper_parent_base<utility::none>
{
};

template <typename Pool>
class wrapper_pool_base
{
public:
  using pool_type = Pool;

  constexpr wrapper_pool_base() noexcept = default;

  constexpr pool_type &
  pool() noexcept
  {
    return pool_;
  }

  constexpr pool_type const &
  pool() const noexcept
  {
    return pool_;
  }

protected:
  template <typename U>
  requires utility::not_this<U, wrapper_pool_base>
  constexpr explicit wrapper_pool_base(U && pool)
      : pool_(std::forward<U>(pool))
  {
  }

private:
  Pool pool_ = {};
};

template <>
class wrapper_pool_base<utility::none>
{
};

template <auto Creator, typename Handle, typename Parent, typename Pool>
struct wrapper_ctor_base : public wrapper_handle_base<Handle>,
                           public wrapper_parent_base<Parent>,
                           public wrapper_pool_base<Pool>
{
  using handle_base = wrapper_handle_base<Handle>;
  using parent_base = wrapper_parent_base<Parent>;
  using pool_base = wrapper_pool_base<Pool>;
  using ctor = wrapper_ctor<Creator>;

  static constexpr bool has_handle = !decltype(utility::is_none(Handle{})){};
  static constexpr bool has_parent = !decltype(utility::is_none(Parent{})){};
  static constexpr bool has_pool = !decltype(utility::is_none(Pool{})){};

  struct member_requirements
  {
    [[nodiscard]] static constexpr bool
    pool() noexcept
    {
      if constexpr (has_pool)
      {
        return has_handle && has_parent;
      }

      return true;
    }

    [[nodiscard]] static constexpr bool
    parent() noexcept
    {
      if constexpr (has_parent)
      {
        return has_handle;
      }

      return true;
    }

    [[nodiscard]] static constexpr bool
    handle() noexcept
    {
      if (has_pool)
      {
        return utility::with_data_and_size<Handle> && has_handle;
      }

      return has_handle;
    }
  };

  static_assert(member_requirements::pool(), "Missing handle or parent");
  static_assert(std::is_same_v<std::remove_cvref_t<Pool>, Pool>,
                "Pool must not be are reference, volatile or const type");
  static_assert(member_requirements::parent(), "Missing handle");
  static_assert(std::is_same_v<std::remove_cvref_t<Parent>, Parent>,
                "Parent must not be are reference, volatile or const type");
  static_assert(member_requirements::handle(), "Missing handle");
  static_assert(std::is_same_v<std::remove_cvref_t<Parent>, Parent>,
                "Handle must not be are reference, volatile or const type");

  constexpr wrapper_ctor_base() noexcept = default;

  // clang-format off

  template <typename Arg, typename... Args>
  requires utility::not_this<Arg, wrapper_ctor_base> && has_handle && 
      has_parent && has_pool 
  constexpr explicit wrapper_ctor_base(Arg && arg, Args &&... args)
      : handle_base(ctor::template create<Handle>(std::forward<Arg>(arg), 
                                                  std::forward<Args>(args)...)),
        parent_base(ctor::parent(std::forward<Arg>(arg),
                                 std::forward<Args>(args)...)),
        pool_base(ctor::pool(std::forward<Arg>(arg),
                            std::forward<Args>(args)...))
  {
  }

  template <typename Arg, typename... Args>
  requires utility::not_this<Arg, wrapper_ctor_base> && has_handle &&
        has_parent && (!has_pool) 
  constexpr explicit wrapper_ctor_base(Arg && arg, Args &&... args)
      : handle_base(ctor::create(std::forward<Arg>(arg),
                                 std::forward<Args>(args)...)),
        parent_base(ctor::parent(std::forward<Arg>(arg),
                                 std::forward<Args>(args)...))
  {
  }

  template <typename Arg, typename... Args>
  requires utility::not_this<Arg, wrapper_ctor_base> && has_handle &&
        (!has_parent) && (!has_pool) 
  constexpr explicit(!std::is_trivial_v<Handle>) 
  wrapper_ctor_base(Arg && arg, Args &&... args)
      : handle_base(ctor::create(std::forward<Arg>(arg), 
                                 std::forward<Args>(args)...))
  {
  }

  // clang-format on
};

template <auto Deleter, auto Creator, typename... Others>
struct wrapper_dtor_base : public wrapper_ctor_base<Creator, Others...>
{
  using base = wrapper_ctor_base<Creator, Others...>;
  using dtor = wrapper_dtor<Deleter>;

  using base::base;

  constexpr wrapper_dtor_base() noexcept = default;

  wrapper_dtor_base(wrapper_dtor_base const & other) noexcept = default;
  wrapper_dtor_base(wrapper_dtor_base && other) noexcept = default;

  wrapper_dtor_base &
  operator=(wrapper_dtor_base const & other) noexcept = default;
  wrapper_dtor_base &
  operator=(wrapper_dtor_base && other) noexcept = default;

  ~wrapper_dtor_base() noexcept
  {
    destroy();
  }

  constexpr void
  release() noexcept
  {
    destroy();

    if constexpr (base::has_handle)
    {
      base::get() = typename base::handle_type();
    }

    if constexpr (base::has_parent)
    {
      base::parent() = typename base::parent_type();
    }

    if constexpr (base::has_pool)
    {
      base::pool() = typename base::pool_type();
    }
  }

  constexpr void
  destroy() const noexcept
  {
    if constexpr (base::has_pool)
    {
      dtor::destroy(base::parent(), base::pool(), base::get());
    }
    else if constexpr (base::has_parent)
    {
      dtor::destroy(base::parent(), base::get());
    }
    else if constexpr (base::has_handle)
    {
      dtor::destroy(base::get());
    }
  }
};

template <auto Creator, typename... Others>
struct wrapper_dtor_base<utility::none{}, Creator, Others...>
    : public wrapper_ctor_base<Creator, Others...>
{
  using base = wrapper_ctor_base<Creator, Others...>;
  using base::base;
};

template <auto Deleter, auto Creator, typename... Others>
struct wrapper_move_base
    : public wrapper_dtor_base<Deleter, Creator, Others...>
{

  using base = wrapper_dtor_base<Deleter, Creator, Others...>;
  using base::base;

  constexpr wrapper_move_base() noexcept = default;

  wrapper_move_base(wrapper_move_base const & other) noexcept = delete;
  wrapper_move_base(wrapper_move_base && other) noexcept
  {
    swap(other);
  }

  wrapper_move_base &
  operator=(wrapper_move_base const & other) noexcept = delete;
  wrapper_move_base &
  operator=(wrapper_move_base && other) noexcept
  {
    swap(other);
    return *this;
  }

  ~wrapper_move_base() noexcept = default;

private:
  constexpr void
  swap(wrapper_move_base & other)
  {
    if constexpr (base::has_handle)
    {
      std::swap(base::get(), other.get());
    }

    if constexpr (base::has_parent)
    {
      std::swap(base::parent(), other.parent());
    }

    if constexpr (base::has_pool)
    {
      std::swap(base::pool(), other.pool());
    }
  }
};

template <auto Creator, typename... Others>
struct wrapper_move_base<utility::none{}, Creator, Others...>
    : public wrapper_dtor_base<utility::none{}, Creator, Others...>
{
  using base = wrapper_dtor_base<utility::none{}, Creator, Others...>;
  using base::base;
};

template <typename... Args>
class wrapper : public wrapper_move_base<
                    find_deleter(utility::pack<Args...>{}),
                    find_creator(utility::pack<Args...>{}),
                    decltype(find_handle(utility::pack<Args...>{})),
                    decltype(find_parent(utility::pack<Args...>{})),
                    decltype(find_pool(utility::pack<Args...>{}))>
{
  using base =
      wrapper_move_base<find_deleter(utility::pack<Args...>{}),
                        find_creator(utility::pack<Args...>{}),
                        decltype(find_handle(utility::pack<Args...>{})),
                        decltype(find_parent(utility::pack<Args...>{})),
                        decltype(find_pool(utility::pack<Args...>{}))>;

public:
  using base::base;
};

} // namespace mvk::types::detail

#endif
