#ifndef MVK_TYPES_DETAIL_WRAPPER_CTOR_HANDLER_HPP_INCLUDED
#define MVK_TYPES_DETAIL_WRAPPER_CTOR_HANDLER_HPP_INCLUDED

#include "utility/pack.hpp"
#include "utility/types.hpp"
#include "validation/validation.hpp"

#include <vulkan/vulkan.h>

namespace mvk::detail
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

template <auto Call, typename = decltype(Call)>
struct creator_traits;

template <auto Call, typename Info, typename Handle>
struct creator_traits<
    Call, VkResult (*)(Info const *, VkAllocationCallbacks const *, Handle *)>
{
  [[nodiscard]] static constexpr Handle
  create(Info const & info) noexcept
  {
    auto handle = Handle();
    Call(&info, nullptr, &handle);
    return handle;
  }
};

template <auto Call, typename Info, typename Handle, typename Parent>
struct creator_traits<Call,
                      VkResult (*)(Parent, Info const *,
                                   VkAllocationCallbacks const *, Handle *)>
{
  [[nodiscard]] static constexpr Handle
  create(Parent const parent, Info const & info) noexcept
  {
    auto handle = Handle();
    Call(parent, &info, nullptr, &handle);
    return handle;
  }

  [[nodiscard]] static constexpr auto
  parent(Parent const parent, [[maybe_unused]] Info const & info) noexcept
  {
    return parent;
  }
};

template <auto Call, typename Info, typename Handle, typename Parent>
struct creator_traits<Call, void (*)(Parent, Info const *, Handle *)>
{
  template <typename HandleBuffer>
  [[nodiscard]] static constexpr HandleBuffer
  create(Parent const parent, Info const & info) noexcept
  {
    auto handle = HandleBuffer(size_from_info(info));
    Call(parent, &info, std::data(handle));
    return handle;
  }

  [[nodiscard]] static constexpr auto
  parent(Parent const parent, [[maybe_unused]] Info const & info) noexcept
  {
    return parent;
  }
};

template <auto Call, typename Info, typename Handle, typename Parent>
struct creator_traits<Call, VkResult (*)(Parent, Info const *, Handle *)>
{
  template <typename HandleBuffer>
  [[nodiscard]] static constexpr HandleBuffer
  create(Parent const parent, Info const & info) noexcept
  {
    auto handle = HandleBuffer(size_from_info(info));
    Call(parent, &info, std::data(handle));
    return handle;
  }

  [[nodiscard]] static constexpr auto
  parent(Parent const parent, [[maybe_unused]] Info const & info) noexcept
  {
    return parent;
  }

  [[nodiscard]] static constexpr auto
  pool([[maybe_unused]] Parent const parent, Info const & info) noexcept
  {
    return pool_from_info(info);
  }
};

template <>
struct creator_traits<validation::setup_debug_messenger>
{
  [[nodiscard]] static constexpr auto
  create(VkInstance const instance)
  {
    return validation::setup_debug_messenger(instance);
  }

  [[nodiscard]] static constexpr auto
  parent(VkInstance const instance)
  {
    return instance;
  }
};

template <>
struct creator_traits<vkCreateGraphicsPipelines>
{
  [[nodiscard]] static constexpr VkPipeline
  create(VkDevice const device,
         VkGraphicsPipelineCreateInfo const & info) noexcept
  {
    auto handle = VkPipeline();
    vkCreateGraphicsPipelines(device, nullptr, 1, &info, nullptr, &handle);
    return handle;
  }

  [[nodiscard]] static constexpr VkDevice
  parent(VkDevice const device,
         [[maybe_unused]] VkGraphicsPipelineCreateInfo const & info) noexcept
  {
    return device;
  }
};

template <>
struct creator_traits<vkGetDeviceQueue>
{
  [[nodiscard]] static constexpr VkQueue
  create(VkDevice const device, uint32_t const index) noexcept
  {
    auto handle = VkQueue();
    vkGetDeviceQueue(device, index, 0, &handle);
    return handle;
  }

  [[nodiscard]] static constexpr VkDevice
  parent(VkDevice const device,
         [[maybe_unused]] uint32_t const index) noexcept
  {
    return device;
  }
};

template <>
struct creator_traits<vkCreateDevice>
{
  [[nodiscard]] static constexpr VkDevice
  create(VkPhysicalDevice const parent, VkDeviceCreateInfo const & info)
  {
    auto handle = VkDevice();
    vkCreateDevice(parent, &info, nullptr, &handle);
    return handle;
  }
};

template <>
struct creator_traits<utility::none{}>
{
  template <typename Handle, typename... Others>
  [[nodiscard]] static constexpr decltype(auto)
  create(Handle && handle, [[maybe_unused]] Others &&... others) noexcept
  {
    return std::forward<Handle>(handle);
  }

  template <typename Handle, typename Parent>
  [[nodiscard]] static constexpr decltype(auto)
  parent([[maybe_unused]] Handle && handle, Parent && parent) noexcept
  {
    return std::forward<Parent>(parent);
  }
};

} // namespace mvk::detail

#endif
