#pragma once

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "ShaderTypes.hpp"
#include "Utility/Badge.hpp"

#include <array>
#include <optional>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

namespace Mvk::Engine {

class Renderer;

struct Context {
#ifndef NDEBUG
  static constexpr auto UseValidation = true;
#else
  static constexpr auto UseValidation = false;
#endif

  static constexpr auto ValidationLayers =
      std::array{"VK_LAYER_KHRONOS_validation"};
  static constexpr auto ValidationInstanceExtensionss =
      std::array{VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
  static constexpr auto DeviceExtensions =
      std::array{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  static constexpr auto MaxFramesInFlight = 2;
  static constexpr size_t DynamicBuffCount = 2;
  static constexpr size_t GarbageBuffCount = 2;

  Context(std::string const &Name, VkExtent2D Size) noexcept;

  Context(Context const &Other) noexcept = delete;
  Context(Context &&Other) noexcept = delete;
  Context &operator=(Context const &Other) noexcept = delete;
  Context &operator=(Context &&Other) noexcept = delete;

  ~Context() noexcept;

  [[nodiscard]] constexpr GLFWwindow *getWindow() const noexcept;

  [[nodiscard]] constexpr VkPhysicalDevice getPhysicalDevice() const noexcept;

  [[nodiscard]] constexpr VkDevice getDevice() const noexcept;

  [[nodiscard]] constexpr VkQueue getPresentQueue() const noexcept;

  [[nodiscard]] constexpr VkQueue getGfxQueue() const noexcept;

  [[nodiscard]] constexpr VkSurfaceFormatKHR getSurfaceFmt() const noexcept;

  [[nodiscard]] constexpr uint32_t getSwapchainImgCount() const noexcept;

  [[nodiscard]] constexpr VkExtent2D getSwapchainExtent() const noexcept;

  [[nodiscard]] constexpr VkDescriptorPool getDescPool() const noexcept;

  [[nodiscard]] constexpr VkDescriptorSetLayout
  getTexDescriptorSetLayout() const noexcept;

  [[nodiscard]] constexpr VkDescriptorSetLayout
  getUboDescSetLayout() const noexcept;

  [[nodiscard]] constexpr size_t getCurrentBuffIdx() const noexcept;

  [[nodiscard]] constexpr VkFramebuffer getCurrentFramebuffer() const noexcept;

  [[nodiscard]] constexpr VkCommandBuffer getCurrentCmdBuff() const noexcept;

  [[nodiscard]] constexpr VkFence getCurrentFrameInFlightFence() const noexcept;

  [[nodiscard]] constexpr VkSampler getTexSampler() const noexcept;

  [[nodiscard]] constexpr VkRenderPass getMainRenderPass() const noexcept;

  [[nodiscard]] constexpr VkSwapchainKHR getSwapchain() const noexcept;

  [[nodiscard]] constexpr uint32_t getCurrentImgIdx() const noexcept;

  [[nodiscard]] constexpr bool isFramebufferResized() const noexcept;

  [[nodiscard]] constexpr VkPipeline getMainPipeline() const noexcept;

  [[nodiscard]] constexpr VkPipelineLayout
  getMainPipelineLayout() const noexcept;

  constexpr void setFramebufferResized(bool State) noexcept;

  [[nodiscard]] constexpr VkSemaphore
  getCurrentImgAvailableSemaphore() const noexcept;

  [[nodiscard]] constexpr VkSemaphore
  getCurrentRenderFinishedSemaphore() const noexcept;

  [[nodiscard]] constexpr std::optional<VkFence>
  getCurrentImgInFlightFence() const noexcept;

  constexpr void setCurrentImgInFlightFence(VkFence Fence) noexcept;

  [[nodiscard]] VkExtent2D getFramebufferSize() const noexcept;

  void updateImgIdx(Utility::Badge<Renderer> Badge) noexcept;

  void updateIdx(Utility::Badge<Renderer> Badge) noexcept;

  using Seconds = float;
  [[nodiscard]] Seconds getCurrentTime() const noexcept;

  void addMemoryToGarbage(VkDeviceMemory Mem) noexcept;

  void
  addDescriptorSetsToGarbage(std::span<VkDescriptorSet const> Sets) noexcept;

  void addBuffersToGarbage(std::span<VkBuffer const> Buffs) noexcept;

  void recreateAfterFramebufferChange() noexcept;

private:
  [[nodiscard]] std::optional<uint32_t>
  acquireNextSwapchainImageIdx() const noexcept;

  void initWindow(std::string const &Name, VkExtent2D Extent) noexcept;
  void initInstace(std::string const &Name) noexcept;
  void initDbgMsngr() noexcept;
  void initSurface() noexcept;
  void selectPhysicalDevice() noexcept;
  void selectSurfaceFmt() noexcept;
  void initDevice() noexcept;
  void initLayouts() noexcept;
  void initPools() noexcept;
  void initSwapchain() noexcept;
  void initDepthImg() noexcept;
  void initFramebuffers() noexcept;
  void initRenderPass() noexcept;
  void initCmdBuffs() noexcept;
  void initShaders() noexcept;
  void initSamplers() noexcept;
  void initPipelines() noexcept;
  void initSync() noexcept;

  void dstrWindow() noexcept;
  void dstrInstance() noexcept;
  void dstrDbgMsngr() noexcept;
  void dstrSurface() noexcept;
  void dstrDevice() noexcept;
  void dstrLayouts() noexcept;
  void dstrPools() noexcept;
  void dstrSwapchain() noexcept;
  void dstrDepthImg() noexcept;
  void dstrFramebuffers() noexcept;
  void dstrRenderPass() noexcept;
  void dstrCmdBuffs() noexcept;
  void dstrShaders() noexcept;
  void dstrSamplers() noexcept;
  void dstrPipelines() noexcept;
  void dstrCurrentGarbageBuffs() noexcept;
  void dstrCurrentGarbageMems() noexcept;
  void dstrCurrentGarbageSets() noexcept;
  void dstrGarbageBuffs() noexcept;
  void dstrGarbageMems() noexcept;
  void dstrGarbageSets() noexcept;
  void dstrSync() noexcept;

  // Window
  GLFWwindow *Window;
  bool FramebufferResized;

  // Instance
  VkInstance Instance;

  // Surface
  VkSurfaceKHR Surface;
  VkSurfaceFormatKHR SurfaceFmt;

  // Debug
  VkDebugUtilsMessengerEXT DbgMsngr;

  // Device
  VkPhysicalDevice PhysicalDevice;
  VkDevice Device;
  uint32_t GfxQueueIdx;
  uint32_t PresentQueueIdx;
  VkQueue GfxQueue;
  VkQueue PresentQueue;

  // Layouts
  VkDescriptorSetLayout UboDescSetLayout;
  VkDescriptorSetLayout TexDescSetLayout;
  VkPipelineLayout MainPipelineLayout;

  // Pools
  VkCommandPool CmdPool;
  VkDescriptorPool DescPool;

  // Swapchain
  uint32_t SwapchainImgCount;
  VkSwapchainKHR Swapchain;
  std::vector<VkImageView> SwapchainImgViews;
  VkExtent2D SwapchainExtent;

  // Depth Image
  VkImage DepthImg;
  VkDeviceMemory DepthImgMem;
  VkImageView DepthImgView;

  // Framebuffers
  std::vector<VkFramebuffer> Framebuffers;

  // RenderPass
  VkRenderPass RenderPass;

  // CommandBuffers
  std::array<VkCommandBuffer, DynamicBuffCount> CmdBuffs;

  // ShaderModules
  VkShaderModule VtxShader;
  VkShaderModule FragShader;

  // Samplers
  VkSampler TexSampler;

  // initPipeline
  VkPipeline MainPipeline;

  // Sync
  std::array<VkSemaphore, MaxFramesInFlight> ImgAvailableSemaphores;
  std::array<VkSemaphore, MaxFramesInFlight> RenderFinishedSemaphores;
  std::array<VkFence, MaxFramesInFlight> FrameInFlightFences;
  std::vector<std::optional<VkFence>> ImgInFlightFences;

  // Garbage
  std::array<std::vector<VkBuffer>, GarbageBuffCount> GarbageBuffs;
  std::array<std::vector<VkDeviceMemory>, GarbageBuffCount> GarbageMems;
  std::array<std::vector<VkDescriptorSet>, GarbageBuffCount>
      GarbageDescriptorSets;

  // Counters
  size_t CurrentFrameIdx = 0;
  size_t CurrentBuffIdx = 0;
  size_t CurrentGarbageIdx = 0;
  uint32_t CurrentImgIdx = 0;
  VkCommandBuffer CurrentCmdBuff = VK_NULL_HANDLE;

  // Start Point
  std::chrono::time_point<std::chrono::high_resolution_clock> StartTime =
      std::chrono::high_resolution_clock::now();
};

[[nodiscard]] constexpr GLFWwindow *Context::getWindow() const noexcept {
  return Window;
}
[[nodiscard]] constexpr VkPhysicalDevice
Context::getPhysicalDevice() const noexcept {
  return PhysicalDevice;
}
[[nodiscard]] constexpr VkDevice Context::getDevice() const noexcept {
  return Device;
}
[[nodiscard]] constexpr VkQueue Context::getPresentQueue() const noexcept {
  return PresentQueue;
}
[[nodiscard]] constexpr VkQueue Context::getGfxQueue() const noexcept {
  return GfxQueue;
}
[[nodiscard]] constexpr VkSurfaceFormatKHR
Context::getSurfaceFmt() const noexcept {
  return SurfaceFmt;
}
[[nodiscard]] constexpr uint32_t
Context::getSwapchainImgCount() const noexcept {
  return SwapchainImgCount;
}
[[nodiscard]] constexpr VkExtent2D
Context::getSwapchainExtent() const noexcept {
  return SwapchainExtent;
}
[[nodiscard]] constexpr VkDescriptorPool Context::getDescPool() const noexcept {
  return DescPool;
}
[[nodiscard]] constexpr VkDescriptorSetLayout
Context::getTexDescriptorSetLayout() const noexcept {
  return TexDescSetLayout;
}
[[nodiscard]] constexpr VkDescriptorSetLayout
Context::getUboDescSetLayout() const noexcept {
  return UboDescSetLayout;
}
[[nodiscard]] constexpr size_t Context::getCurrentBuffIdx() const noexcept {
  return CurrentBuffIdx;
}
[[nodiscard]] constexpr VkFramebuffer
Context::getCurrentFramebuffer() const noexcept {
  return Framebuffers[CurrentImgIdx];
}
[[nodiscard]] constexpr VkCommandBuffer
Context::getCurrentCmdBuff() const noexcept {
  return CurrentCmdBuff;
}
[[nodiscard]] constexpr VkFence
Context::getCurrentFrameInFlightFence() const noexcept {
  return FrameInFlightFences[CurrentFrameIdx];
}
[[nodiscard]] constexpr VkSampler Context::getTexSampler() const noexcept {
  return TexSampler;
}
[[nodiscard]] constexpr VkRenderPass
Context::getMainRenderPass() const noexcept {
  return RenderPass;
}
[[nodiscard]] constexpr VkSwapchainKHR Context::getSwapchain() const noexcept {
  return Swapchain;
}
[[nodiscard]] constexpr uint32_t Context::getCurrentImgIdx() const noexcept {
  return CurrentImgIdx;
}
[[nodiscard]] constexpr bool Context::isFramebufferResized() const noexcept {
  return FramebufferResized;
}
[[nodiscard]] constexpr VkPipeline Context::getMainPipeline() const noexcept {
  return MainPipeline;
}
[[nodiscard]] constexpr VkPipelineLayout
Context::getMainPipelineLayout() const noexcept {
  return MainPipelineLayout;
}

constexpr void Context::setFramebufferResized(bool State) noexcept {
  FramebufferResized = State;
}

[[nodiscard]] constexpr VkSemaphore
Context::getCurrentImgAvailableSemaphore() const noexcept {
  return ImgAvailableSemaphores[CurrentFrameIdx];
}

[[nodiscard]] constexpr VkSemaphore
Context::getCurrentRenderFinishedSemaphore() const noexcept {
  return RenderFinishedSemaphores[CurrentFrameIdx];
}

[[nodiscard]] constexpr std::optional<VkFence>
Context::getCurrentImgInFlightFence() const noexcept {
  return ImgInFlightFences[CurrentImgIdx];
}

constexpr void Context::setCurrentImgInFlightFence(VkFence Fence) noexcept {
  ImgInFlightFences[CurrentImgIdx] = Fence;
}

} // namespace Mvk::Engine

