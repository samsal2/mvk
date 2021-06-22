#ifndef MVK_WRAPPER_OWNER_CREATE_HPP_INCLUDED
#define MVK_WRAPPER_OWNER_CREATE_HPP_INCLUDED

#include "wrapper/fwd.hpp"

namespace mvk::wrapper
{

namespace creator
{

struct owner_create
{
};

} // namespace creator

template <auto Call, typename Wrapper>
class owner_create;

template <typename... Args>
constexpr auto
creator_selector([[maybe_unused]] creator::owner_create option)
{
  using wrapper = any_wrapper<Args...>;
  constexpr auto call = select<options::creator_call>(Args{}...);
  static_assert(!utility::is_none(call), "Expected creator_call option");

  return detail::select<owner_create<call, wrapper>>{};
}

template <typename Wrapper>
class owner_create<vkCreateInstance, Wrapper>
{
  using wrapper_type = Wrapper;
  static constexpr auto creator_call = vkCreateInstance;

public:
  [[nodiscard]] static constexpr wrapper_type
  create(VkInstanceCreateInfo const & info) noexcept
  {
    auto handle = VkInstance();
    creator_call(&info, nullptr, &handle);
    return wrapper_type(handle);
  }
};

template <typename Wrapper>
class owner_create<vkCreateDevice, Wrapper>
{
  using wrapper_type = Wrapper;
  static constexpr auto creator_call = vkCreateDevice;

public:
  [[nodiscard]] static constexpr wrapper_type
  create(VkPhysicalDevice const physical_device,
         VkDeviceCreateInfo const & info) noexcept
  {
    auto handle = VkDevice();
    creator_call(physical_device, &info, nullptr, &handle);
    return wrapper_type(handle);
  }
};

} // namespace mvk::wrapper

#endif
