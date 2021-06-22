#ifndef MVK_TYPES_DETAIL_UNIQUE_HPP_INCLUDED
#define MVK_TYPES_DETAIL_UNIQUE_HPP_INCLUDED

#include "utility/compressed_pair.hpp"
#include "wrapper/fwd.hpp"

#include <utility>

namespace mvk::wrapper
{
namespace storage
{
struct unique
{
};

}; // namespace storage

template <typename Handle, typename Deleter>
class unique;

template <typename... Args>
constexpr auto
storage_selector([[maybe_unused]] storage::unique option) noexcept
{
  auto arg = select<options::deleter>(Args{}...);
  static_assert(!utility::is_none(arg), "Expected a deleter option");

  auto found = deleter_selector<Args...>(arg);
  using deleter = detail::selected_t<decltype(found)>;

  using handle = decltype(select<options::handle>(Args{}...));
  static_assert(!utility::is_none(handle{}), "Expected a handle option");

  return detail::select<unique<handle, deleter>>{};
}

template <typename Handle, typename Deleter>
class unique
{
public:
  using deleter_type = Deleter;
  using handle_type = Handle;

  constexpr unique() noexcept = default;

  template <typename HandleArg, typename DeleterArg = deleter_type>
  constexpr explicit unique(HandleArg && handle,
                            DeleterArg && deleter = deleter_type()) noexcept
      : container_(std::forward<HandleArg>(handle),
                   std::forward<DeleterArg>(deleter))
  {
  }

  unique(unique const & other) noexcept = delete;
  unique(unique && other) noexcept
  {
    swap(other);
  }

  unique &
  operator=(unique const & other) noexcept = delete;
  unique &
  operator=(unique && other) noexcept
  {
    swap(other);
    return *this;
  }

  ~unique() noexcept
  {
    deleter().destroy(get());
  }

  [[nodiscard]] constexpr handle_type const &
  get() const noexcept
  {
    return container_.first();
  }

  [[nodiscard]] constexpr handle_type &
  get() noexcept
  {
    return container_.first();
  }

  [[nodiscard]] constexpr deleter_type const &
  deleter() const noexcept
  {
    return container_.second();
  }

  [[nodiscard]] constexpr deleter_type &
  deleter() noexcept
  {
    return container_.second();
  }

private:
  constexpr void
  swap(unique & other) noexcept
  {
    std::swap(get(), other.get());
    std::swap(deleter(), other.deleter());
  }

  utility::compressed_pair<handle_type, deleter_type> container_ = {};
};

} // namespace mvk::wrapper
#endif
