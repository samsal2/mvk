#ifndef MVK_WRAPPER_HANDLE_ONLY_HPP_INCLUDED
#define MVK_WRAPPER_HANDLE_ONLY_HPP_INCLUDED

#include "utility/concepts.hpp"
#include "wrapper/fwd.hpp"

namespace mvk::wrapper
{
namespace storage
{
struct handle_only
{
};

} // namespace storage

template <typename Handle>
class handle_only;

template <typename... Args>
constexpr auto
storage_selector([[maybe_unused]] storage::handle_only option) noexcept
{
  using handle = decltype(select<options::handle>(Args{}...));
  static_assert(!utility::is_none(handle{}), "Expected handle option");

  return detail::select<handle_only<handle>>{};
}

template <typename Handle>
class handle_only
{
  using handle_type = Handle;

public:
  constexpr handle_only() noexcept = default;

  template <typename HandleArg>
  requires utility::not_this<Handle, handle_only>
  constexpr handle_only(HandleArg && handle) noexcept
      : handle_(std::forward<HandleArg>(handle))
  {
  }

  [[nodiscard]] constexpr handle_type const &
  get() const noexcept
  {
    return handle_;
  }

  [[nodiscard]] constexpr handle_type &
  get() noexcept
  {
    return handle_;
  }

private:
  handle_type handle_;
};

} // namespace mvk::wrapper

#endif
