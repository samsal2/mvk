#ifndef MVK_WRAPPER_OBJECT_FREE_HPP_INCLUDED
#define MVK_WRAPPER_OBJECT_FREE_HPP_INCLUDED

#include "wrapper/fwd.hpp"

#include <utility>
#include <vulkan/vulkan.h>

namespace mvk::wrapper
{
namespace deleter
{
struct object_free
{
};

} // namespace deleter

template <auto Call, typename Parent, typename Pool>
class object_free;

template <typename... Args>
constexpr auto
deleter_selector([[maybe_unused]] deleter::object_free option) noexcept
{
  using parent = decltype(select<options::parent>(Args{}...));
  static_assert(!utility::is_none(parent{}), "Expected a parent option");

  using pool = decltype(select<options::pool>(Args{}...));
  static_assert(!utility::is_none(pool{}), "Expected a pool option");

  constexpr auto deleter_call = select<options::deleter_call>(Args{}...);
  static_assert(!utility::is_none(deleter_call),
                "Expected a deleter_call option");

  return detail::select<object_free<deleter_call, parent, pool>>{};
}

template <auto Call, typename Parent, typename Pool>
class object_free
{
  using parent_type = Parent;
  using pool_type = Pool;
  static constexpr auto deleter_call = Call;

public:
  constexpr object_free() noexcept = default;

  template <typename ParentArg, typename PoolArg>
  constexpr object_free(ParentArg && parent, PoolArg && pool) noexcept
      : parent_(std::forward<ParentArg>(parent)),
        pool_(std::forward<PoolArg>(pool))
  {
  }

  template <typename Handle>
  constexpr void
  destroy(Handle handle)
  {
    if (parent_ != VK_NULL_HANDLE && pool_ != VK_NULL_HANDLE)
    {
      deleter_call(parent_, pool_, 1, &handle);
    }
  }

private:
  parent_type parent_;
  pool_type pool_;
};

}; // namespace mvk::wrapper

#endif
