#ifndef MVK_TYPES_DETAIL_CREATOR_HPP_INCLUDED
#define MVK_TYPES_DETAIL_CREATOR_HPP_INCLUDED

#include "types/common.hpp"
#include "utility/common.hpp"
#include "utility/concepts.hpp"
#include "utility/types.hpp"
#include "validation/validation.hpp"

namespace mvk::types::detail
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
struct create_handler;

template <auto Call, typename Info, typename Handle>
struct create_handler<
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
struct create_handler<Call,
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
struct create_handler<Call, void (*)(Parent, Info const *, Handle *)>
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
struct create_handler<Call, VkResult (*)(Parent, Info const *, Handle *)>
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
struct create_handler<glfwCreateWindowSurface>
{
  [[nodiscard]] static constexpr VkSurfaceKHR
  create(VkInstance const instance, GLFWwindow * window) noexcept
  {
    auto handle = VkSurfaceKHR();
    glfwCreateWindowSurface(instance, window, nullptr, &handle);
    return handle;
  }

  [[nodiscard]] static constexpr VkInstance
  parent(VkInstance const instance,
         [[maybe_unused]] GLFWwindow * window) noexcept
  {
    return instance;
  }
};

template <>
struct create_handler<validation::setup_debug_messenger>
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
struct create_handler<vkCreateGraphicsPipelines>
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
struct create_handler<vkGetDeviceQueue>
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
struct create_handler<vkCreateDevice>
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
struct create_handler<utility::none{}>
{
  template <typename Handle>
  [[nodiscard]] static constexpr decltype(auto)
  create(Handle && handle) noexcept
  {
    return std::forward<Handle>(handle);
  }
};

} // namespace mvk::types::detail

#endif
