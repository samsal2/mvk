#ifndef MVK_WRAPPER_CREATED_HPP_INCLUDED
#define MVK_WRAPPER_CREATED_HPP_INCLUDED

#include "validation/validation.hpp"
#include "wrapper/fwd.hpp"

#include <vulkan/vulkan.h>

namespace mvk::wrapper
{
namespace creator
{
struct created
{
};

} // namespace creator

template <auto Call, typename Wrapper>
class created;

template <typename... Args>
constexpr auto
creator_selector([[maybe_unused]] creator::created option)
{
  using wrapper = any_wrapper<Args...>;

  /*
  using storage_type = decltype(select<options::storage>(Args{}...));
  static_assert(std::is_same_v<storage_type, storage::unique>,
                "Using option<creator::created> requires "
                "option<storage::unique> to be set");
  */

  constexpr auto call = select<options::creator_call>(Args{}...);
  static_assert(!utility::is_none(call), "Expected creator_call option");

  return detail::select<created<call, wrapper>>{};
}

template <auto Call, typename Wrapper>
class created
{
  using wrapper_type = Wrapper;
  static constexpr auto create_call = Call;

public:
  template <typename Parent, typename Info>
  [[nodiscard]] static constexpr wrapper_type
  create(Parent const parent, Info const & info) noexcept
  {
    using handle_type = typename wrapper_type::handle_type;
    auto handle = handle_type();

    create_call(parent, &info, nullptr, &handle);

    using deleter_type = typename wrapper_type::deleter_type;
    return wrapper_type(handle, deleter_type(parent));
  }
};

template <typename Wrapper>
class created<validation::setup_debug_messenger, Wrapper>
{
  using wrapper_type = Wrapper;
  static constexpr auto create_call = validation::setup_debug_messenger;

public:
  [[nodiscard]] static constexpr wrapper_type
  create(VkInstance const parent) noexcept
  {
    auto handle = create_call(parent);

    using deleter_type = typename wrapper_type::deleter_type;
    return wrapper_type(handle, deleter_type(parent));
  }
};

template <typename Wrapper>
class created<vkCreateGraphicsPipelines, Wrapper>
{
  using wrapper_type = Wrapper;
  static constexpr auto creator_call = vkCreateGraphicsPipelines;

public:
  [[nodiscard]] static constexpr wrapper_type
  create(VkDevice const device,
         VkGraphicsPipelineCreateInfo const & info) noexcept
  {
    auto handle = VkPipeline();
    creator_call(device, nullptr, 1, &info, nullptr, &handle);

    using deleter_type = typename wrapper_type::deleter_type;
    return wrapper_type(handle, deleter_type(device));
  }
};

template <typename Wrapper>
class created<vkCreateDevice, Wrapper>
{
  using wrapper_type = Wrapper;
  static constexpr auto creator_call = vkCreateDevice;

public:
  [[nodiscard]] static constexpr wrapper_type
  create(VkPhysicalDevice const parent, VkDeviceCreateInfo const & info)
  {
    auto handle = VkDevice();
    creator_call(parent, &info, nullptr, &handle);
    return wrapper_type(handle);
  }
};

} // namespace mvk::wrapper

#endif
