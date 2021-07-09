#pragma once

#include "Engine/Model.hpp"
#include "GLFW/glfw3.h"
#include "Utility/Macros.hpp"

#include <array>
#include <iostream>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

namespace Mvk::Engine
{
  using ModelID = size_t;

  class VulkanRenderer
  {
  public:
    static constexpr auto DynamicBuffCount  = 2;
    static constexpr auto MaxFramesInFlight = 2;

    // Expects VulkanContext to be initialized
    VulkanRenderer() noexcept;
    MVK_DEFINE_NON_COPYABLE( VulkanRenderer );
    MVK_DEFINE_NON_MOVABLE( VulkanRenderer );
    ~VulkanRenderer() noexcept;

    // TODO(samuel): remove model generation from renderer
    // For now it just loads the same model everytime
    // The renderer shouldn't take care of this but for now it will
    [[nodiscard]] ModelID loadModel() noexcept;

    void beginDraw() noexcept;

    void drawModel( ModelID ID ) noexcept;

    void endDraw() noexcept;

    bool isDone() const noexcept
    {
      return glfwWindowShouldClose( VulkanContext::the().getWindow() ) != 0;
    }

  private:
    void updateImgIdx() noexcept;
    void recreateAfterFramebufferChange() noexcept;

    void initLayouts() noexcept;
    void initPools() noexcept;
    void initSwapchain() noexcept;
    void initDepthImg() noexcept;
    void initFramebuffers() noexcept;
    void initRenderPass() noexcept;
    void initCmdBuffs() noexcept;
    void initShaders() noexcept;
    void initPipelines() noexcept;
    void initSync() noexcept;

    void dstrLayouts() noexcept;
    void dstrPools() noexcept;
    void dstrSwapchain() noexcept;
    void dstrDepthImg() noexcept;
    void dstrFramebuffers() noexcept;
    void dstrRenderPass() noexcept;
    void dstrCmdBuffs() noexcept;
    void dstrShaders() noexcept;
    void dstrPipelines() noexcept;
    void dstrSync() noexcept;

    // Layouts
    VkDescriptorSetLayout                         UboTexDescSetLayout;
    VkPipelineLayout                              MainPipelineLayout;
    //
    // Pools
    VkCommandPool                                 CmdPool;
    VkDescriptorPool                              DescPool;
    //
    // Swapchain
    uint32_t                                      SwapchainImgCount;
    VkSwapchainKHR                                Swapchain;
    std::vector<VkImageView>                      SwapchainImgViews;
    VkExtent2D                                    SwapchainExtent;
    //
    // Depth Image
    VkImage                                       DepthImg;
    VkDeviceMemory                                DepthImgMem;
    VkImageView                                   DepthImgView;
    //
    // Framebuffers
    std::vector<VkFramebuffer>                    Framebuffers;
    //
    // RenderPass
    VkRenderPass                                  RenderPass;
    //
    // CommandBuffers
    std::array<VkCommandBuffer, DynamicBuffCount> CmdBuffs;
    //
    // ShaderModules
    VkShaderModule                                VtxShader;
    VkShaderModule                                FragShader;
    //
    // initPipeline
    VkPipeline                                    MainPipeline;
    //
    // Sync
    std::array<VkSemaphore, MaxFramesInFlight>    ImgAvailableSemaphores;
    std::array<VkSemaphore, MaxFramesInFlight>    RenderFinishedSemaphores;
    std::array<VkFence, MaxFramesInFlight>        FrameInFlightFences;
    std::vector<std::optional<VkFence>>           ImgInFlightFences;
    //
    // Counters
    size_t                                        CurrentFrameIdx = 0;
    size_t                                        CurrentBuffIdx  = 0;
    uint32_t                                      CurrentImgIdx   = 0;
    VkCommandBuffer                               CurrentCmdBuff  = VK_NULL_HANDLE;
    //
    // TODO(samsal): For now renderer take care of storing the models
    std::vector<VkDescriptorSet>                  ModelDescSets;
    std::vector<std::unique_ptr<Model>>           Models;
  };

}  // namespace Mvk::Engine
