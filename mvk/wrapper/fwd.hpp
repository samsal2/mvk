#ifndef MVK_WRAPPER_FWD_HPP_INCLUDED
#define MVK_WRAPPER_FWD_HPP_INCLUDED

#include "utility/pack.hpp"

#include <type_traits>
#include <vulkan/vulkan.h>

namespace mvk::wrapper
{
template <typename... Args>
class any_wrapper;

namespace detail
{
constexpr uint32_t
size_from_info(VkCommandBufferAllocateInfo const & info)
{
  return info.commandBufferCount;
}
constexpr VkCommandPool
pool_from_info(VkCommandBufferAllocateInfo const & info)
{
  return info.commandPool;
}

constexpr uint32_t
size_from_info(VkDescriptorSetAllocateInfo const & info)
{
  return info.descriptorSetCount;
}

constexpr VkDescriptorPool
pool_from_info(VkDescriptorSetAllocateInfo const & info)
{
  return info.descriptorPool;
}

template <typename Found>
struct select
{
};

template <typename Found>
struct selected
{
  static_assert(!std::is_same_v<Found, Found>, "type is not a found type");
};

template <typename Found>
struct selected<select<Found>>
{
  using type = Found;
};

template <typename Selected>
using selected_t = typename selected<Selected>::type;

}; // namespace detail

namespace options
{
template <typename Deleter>
struct deleter
{
};

template <typename Creator>
struct creator
{
};

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

template <typename Storage>
struct storage
{
};

template <auto Call>
struct deleter_call
{
};

template <auto Call>
struct creator_call
{
};

} // namespace options

template <typename T, typename... Args>
constexpr auto
storage_selector([[maybe_unused]] T t) noexcept
{
  static_assert(!std::is_same_v<T, T>, "invalid storage option");
}

template <typename T, typename... Args>
constexpr auto
deleter_selector([[maybe_unused]] T t) noexcept
{
  static_assert(!std::is_same_v<T, T>, "invalid deleter option");
}

template <typename T, typename... Args>
constexpr auto
creator_selector([[maybe_unused]] T t) noexcept
{
  static_assert(!std::is_same_v<T, T>, "invalid deleter option");
}

template <typename... Args>
constexpr auto
creator_selector([[maybe_unused]] utility::none t) noexcept
{
  return detail::select<utility::none>{};
}

template <template <typename> typename Tag, typename... Args>
constexpr auto
select([[maybe_unused]] Args... opt)
{
  return utility::unpack_tag(utility::find_if(utility::tagged_with<Tag>(),
                                              utility::pack<Args...>{}));
}

template <template <auto> typename Tag, typename... Args>
constexpr auto
select([[maybe_unused]] Args... opt)
{
  return utility::unpack_tag(utility::find_if(utility::tagged_with<Tag>(),
                                              utility::pack<Args...>{}));
}

} // namespace mvk::wrapper

#endif
