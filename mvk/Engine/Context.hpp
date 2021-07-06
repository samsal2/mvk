#ifndef MVK_ENGINE_CONTEXT_HPP_INCLUDED
#define MVK_ENGINE_CONTEXT_HPP_INCLUDED

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "ShaderTypes.hpp"
#include "Utility/Badge.hpp"
#include "Utility/Slice.hpp"

#include <array>
#include <optional>
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

  [[nodiscard]] GLFWwindow *getWindow() const noexcept { return Window; }
  [[nodiscard]] VkPhysicalDevice getPhysicalDevice() const noexcept {
    return PhysicalDevice;
  }
  [[nodiscard]] VkDevice getDevice() const noexcept { return Device; }
  [[nodiscard]] VkQueue getPresentQueue() const noexcept {
    return PresentQueue;
  }
  [[nodiscard]] VkQueue getGfxQueue() const noexcept { return GfxQueue; }
  [[nodiscard]] VkSurfaceFormatKHR getSurfaceFmt() const noexcept {
    return SurfaceFmt;
  }
  [[nodiscard]] uint32_t getSwapchainImgCount() const noexcept {
    return SwapchainImgCount;
  }
  [[nodiscard]] VkExtent2D getSwapchainExtent() const noexcept {
    return SwapchainExtent;
  }
  [[nodiscard]] VkDescriptorPool getDescPool() const noexcept {
    return DescPool;
  }
  [[nodiscard]] VkDescriptorSetLayout
  getTexDescriptorSetLayout() const noexcept {
    return TexDescSetLayout;
  }
  [[nodiscard]] VkDescriptorSetLayout getUboDescSetLayout() const noexcept {
    return UboDescSetLayout;
  }
  [[nodiscard]] size_t getCurrentBuffIdx() const noexcept {
    return CurrentBuffIdx;
  }
  [[nodiscard]] VkFramebuffer getCurrentFramebuffer() const noexcept {
    return Framebuffers[CurrentImgIdx];
  }
  [[nodiscard]] VkCommandBuffer getCurrentCmdBuff() const noexcept {
    return CurrentCmdBuff;
  }
  [[nodiscard]] VkFence getCurrentFrameInFlightFence() const noexcept {
    return FrameInFlightFences[CurrentFrameIdx];
  }
  [[nodiscard]] VkSampler getTexSampler() const noexcept { return TexSampler; }
  [[nodiscard]] VkRenderPass getMainRenderPass() const noexcept {
    return RenderPass;
  }
  [[nodiscard]] VkSwapchainKHR getSwapchain() const noexcept {
    return Swapchain;
  }
  [[nodiscard]] uint32_t getCurrentImgIdx() const noexcept {
    return CurrentImgIdx;
  }
  [[nodiscard]] bool isFramebufferResized() const noexcept {
    return FramebufferResized;
  }
  [[nodiscard]] VkPipeline getMainPipeline() const noexcept {
    return MainPipeline;
  }
  [[nodiscard]] VkPipelineLayout getMainPipelineLayout() const noexcept {
    return MainPipelineLayout;
  }
  void setFramebufferResized(bool State) noexcept {
    FramebufferResized = State;
  }

  [[nodiscard]] VkSemaphore getCurrentImgAvailableSemaphore() const noexcept {
    return ImgAvailableSemaphores[CurrentFrameIdx];
  }

  [[nodiscard]] VkSemaphore getCurrentRenderFinishedSemaphore() const noexcept {
    return RenderFinishedSemaphores[CurrentFrameIdx];
  }

  [[nodiscard]] std::optional<VkFence>
  getCurrentImginFlightFence() const noexcept {
    return ImgInFlightFences[CurrentImgIdx];
  }

  void setCurrentImgInFlightFence(VkFence Fence) noexcept {
    ImgInFlightFences[CurrentImgIdx] = Fence;
  }
  [[nodiscard]] VkExtent2D getFramebufferSize() const noexcept;

  void updateImgIdx(Utility::Badge<Renderer> Badge) noexcept;
  void updateIdx(Utility::Badge<Renderer> Badge) noexcept;

  using Seconds = float;
  [[nodiscard]] Seconds getCurrentTime() const noexcept;

  void addMemoryToGarbage(VkDeviceMemory Mem) noexcept;
  void addDescriptorSetsToGarbage(
      Utility::Slice<VkDescriptorSet const> Sets) noexcept;
  void addBuffersToGarbage(Utility::Slice<VkBuffer const> Buffs) noexcept;

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

} // namespace Mvk::Engine

#endif
