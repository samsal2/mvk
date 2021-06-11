#ifndef MVK_VK_TYPES_WRAPPER_HPP_INCLUDED
#define MVK_VK_TYPES_WRAPPER_HPP_INCLUDED

#include "utility/types.hpp"
#include "utility/unique_wrapper.hpp"
#include "utility/verify.hpp"
#include "vk_types/detail/deleter.hpp"

namespace mvk::vk_types::detail
{

template <typename Handle, auto DeleterCall>
class wrapper
{
public:
  using handle_type = Handle;

private:
  using deleter_type = deleter<DeleterCall>;

  static constexpr auto is_allocation = deleter_type::is_allocation::value;
  static constexpr auto default_deleter =
    std::is_default_constructible_v<deleter_type>;

public:
  using container_type = detail::get_container_t<Handle, is_allocation>;

  constexpr wrapper() noexcept = default;

  template <typename... Us>
  [[nodiscard]] static constexpr deleter_type
  make_deleter(Us &&... args) noexcept
  {
    return deleter_type(std::forward<Us>(args)...);
  }

  template <typename U>
  constexpr explicit wrapper(container_type container, U && deleter) noexcept
    : container_(std::move(container), std::forward<U>(deleter))
  {
  }

  [[nodiscard]] constexpr utility::const_trivial_t<container_type>
  get() const noexcept
  {
    return container_.get();
  }

  template <typename U = deleter_type>
  [[nodiscard]] constexpr decltype(std::declval<U &>().parent())
  parent() const noexcept
  {
    return container_.deleter().parent();
  }

  template <typename U = deleter_type>
  [[nodiscard]] constexpr decltype(std::declval<U &>().pool())
  pool() const noexcept
  {
    return container_.deleter().pool();
  }

  constexpr void
  release() noexcept
  {
    container_ = utility::unique_wrapper<container_type, deleter_type>();
  }

protected:
  constexpr container_type &
  reference() noexcept
  {
    return container_.get();
  }

private:
  utility::unique_wrapper<container_type, deleter_type> container_;
};

template <typename First, typename... Others>
constexpr decltype(std::declval<First &>().parent())
find_parent(First const & val, [[maybe_unused]] Others const &... vals)
{
  auto parent = val.parent();
  MVK_VERIFY(((vals.parent() == parent) && ...));
  return parent;
}

} // namespace mvk::vk_types::detail

#endif
