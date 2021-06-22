#ifndef MVK_TYPES_DETAIL_OBJECT_DESTROY_HPP_INCLUDED
#define MVK_TYPES_DETAIL_OBJECT_DESTROY_HPP_INCLUDED

#include "utility/concepts.hpp"
#include "wrapper/fwd.hpp"

#include <utility>
#include <vulkan/vulkan.h>

namespace mvk::wrapper
{

namespace deleter
{
struct object_destroy
{
};

} // namespace deleter

template <auto Call, typename Parent>
class object_destroy;

template <typename... Args>
constexpr auto
deleter_selector([[maybe_unused]] deleter::object_destroy option) noexcept
{
  using parent = decltype(select<options::parent>(Args{}...));
  static_assert(!utility::is_none(parent{}), "Expected parent option");

  constexpr auto deleter_call = select<options::deleter_call>(Args{}...);
  static_assert(!utility::is_none(deleter_call),
                "Expected deleter_call option");

  return detail::select<object_destroy<deleter_call, parent>>{};
}

template <auto Call, typename Parent>
class object_destroy
{
  using parent_type = Parent;
  static constexpr auto deleter_call = Call;

public:
  constexpr object_destroy() noexcept = default;

  template <typename ParentArg>
  requires utility::not_this<ParentArg, object_destroy>
  constexpr explicit object_destroy(ParentArg && parent) noexcept
      : parent_(std::forward<ParentArg>(parent))
  {
  }

  [[nodiscard]] constexpr parent_type const &
  parent() const noexcept
  {
    return parent_;
  }

  template <typename Handle>
  constexpr void
  destroy(Handle handle)
  {
    if (parent_ != VK_NULL_HANDLE)
    {
      deleter_call(parent_, handle, nullptr);
    }
  }

private:
  parent_type parent_;
};

}; // namespace mvk::wrapper

#endif
