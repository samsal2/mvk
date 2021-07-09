#pragma once

#include "Detail/Helpers.hpp"
#include "GLFW/glfw3.h"
#include "Utility/Macros.hpp"
#include "Utility/Singleton.hpp"

#include <vulkan/vulkan.h>

namespace Mvk::Engine
{
  using QueueFamilyIdx = uint32_t;

  // TODO(samuel) Vulkan Context and Renderer can be in the same class tbh

  class VulkanContext  // NOLINT(cppcoreguidelines-special-member-functions, hicpp-special-member-functions)
                       // Singleton already disables move and copy
    : public Utility::Singleton<VulkanContext>
  {
  public:
#ifndef NDEBUG
    static constexpr auto UseValidation = true;
#else
    static constexpr auto UseValidation = false;
#endif

    using Utility::Singleton<VulkanContext>::Singleton;

    using Seconds = float;

    static constexpr auto   ValidationLayers              = std::array{ "VK_LAYER_KHRONOS_validation" };
    static constexpr auto   ValidationInstanceExtensionss = std::array{ VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
    static constexpr auto   DeviceExtensions              = std::array{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    static constexpr auto   MaxFramesInFlight             = 2;
    static constexpr size_t DynamicBuffCount              = 2;
    static constexpr size_t GarbageBuffCount              = 2;

    struct Extent
    {
      int Width;
      int Height;
    };

    void initialize( std::string const & Name, Extent Extent );

    [[nodiscard]] constexpr GLFWwindow *       getWindow() const noexcept;
    [[nodiscard]] constexpr VkDevice           getDevice() const noexcept;
    [[nodiscard]] constexpr VkPhysicalDevice   getPhysicalDevice() const noexcept;
    [[nodiscard]] constexpr VkSurfaceFormatKHR getSurfaceFmt() const noexcept;
    [[nodiscard]] constexpr VkQueue            getGraphicsQueue() const noexcept;
    [[nodiscard]] constexpr VkQueue            getPresentQueue() const noexcept;
    [[nodiscard]] constexpr QueueFamilyIdx     getGraphicsQueueFamilyIdx() const noexcept;
    [[nodiscard]] constexpr QueueFamilyIdx     getPresentQueueFamilyIdx() const noexcept;
    [[nodiscard]] constexpr bool               getIsFramebufferResized() const noexcept;
    [[nodiscard]] constexpr VkRenderPass       getRenderPass() const noexcept;
    [[nodiscard]] constexpr VkCommandPool      getCommandPool() const noexcept;
    [[nodiscard]] constexpr VkDescriptorPool   getDescriptorPool() const noexcept;
    [[nodiscard]] constexpr VkSurfaceKHR       getSurface() const noexcept;
    constexpr void                             setIsFramebufferResized( bool State ) noexcept;

    [[nodiscard]] VkExtent2D getFramebufferSize() const noexcept;
    [[nodiscard]] float      getCurrentTime() const noexcept;

    void shutdown() noexcept;

  private:
    void initWindow( std::string const & Name, Extent Extent ) noexcept;
    void initInstace( std::string const & Name ) noexcept;
    void initDbgMsngr() noexcept;
    void initSurface() noexcept;
    void selectPhysicalDevice() noexcept;
    void selectSurfaceFmt() noexcept;
    void initDevice() noexcept;

    void dstrWindow() noexcept;
    void dstrInstance() noexcept;
    void dstrDbgMsngr() noexcept;
    void dstrSurface() noexcept;
    void dstrDevice() noexcept;

    GLFWwindow *             Window;
    bool                     IsFramebufferResized;
    //
    // Instance
    VkInstance               Instance;
    //
    // Surface
    VkSurfaceKHR             Surface;
    VkSurfaceFormatKHR       SurfaceFmt;
    //
    // Debug
    VkDebugUtilsMessengerEXT DbgMsngr;
    //
    // Device
    VkPhysicalDevice         PhysicalDevice;
    VkDevice                 Device;
    uint32_t                 GfxQueueIdx;
    uint32_t                 PresentQueueIdx;
    VkQueue                  GfxQueue;
    VkQueue                  PresentQueue;
    VkRenderPass             RenderPass;

    std::chrono::time_point<std::chrono::high_resolution_clock> StartTime = std::chrono::high_resolution_clock::now();
  };

  [[nodiscard]] constexpr GLFWwindow * VulkanContext::getWindow() const noexcept
  {
    return Window;
  }

  [[nodiscard]] constexpr VkDevice VulkanContext::getDevice() const noexcept
  {
    return Device;
  }

  [[nodiscard]] constexpr VkPhysicalDevice VulkanContext::getPhysicalDevice() const noexcept
  {
    return PhysicalDevice;
  }

  [[nodiscard]] constexpr VkSurfaceFormatKHR VulkanContext::getSurfaceFmt() const noexcept
  {
    return SurfaceFmt;
  }

  [[nodiscard]] constexpr VkQueue VulkanContext::getGraphicsQueue() const noexcept
  {
    return GfxQueue;
  }

  [[nodiscard]] constexpr VkQueue VulkanContext::getPresentQueue() const noexcept
  {
    return PresentQueue;
  }

  [[nodiscard]] constexpr QueueFamilyIdx VulkanContext::getGraphicsQueueFamilyIdx() const noexcept
  {
    return GfxQueueIdx;
  }

  [[nodiscard]] constexpr QueueFamilyIdx VulkanContext::getPresentQueueFamilyIdx() const noexcept
  {
    return PresentQueueIdx;
  }

  [[nodiscard]] constexpr bool VulkanContext::getIsFramebufferResized() const noexcept
  {
    return IsFramebufferResized;
  }

  [[nodiscard]] constexpr VkRenderPass VulkanContext::getRenderPass() const noexcept
  {
    return RenderPass;
  }

  [[nodiscard]] constexpr VkSurfaceKHR VulkanContext::getSurface() const noexcept
  {
    return Surface;
  }

  constexpr void VulkanContext::setIsFramebufferResized( bool State ) noexcept
  {
    IsFramebufferResized = State;
  }

}  // namespace Mvk::Engine