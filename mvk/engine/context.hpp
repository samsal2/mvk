#ifndef MVK_ENGINE_CONTEXT_HPP_INCLUDED
#define MVK_ENGINE_CONTEXT_HPP_INCLUDED

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "shader_types.hpp"
#include "utility/slice.hpp"

#include <array>
#include <vector>
#include <vulkan/vulkan.h>

namespace mvk::engine
{
  struct Context
  {
#ifndef NDEBUG
    static constexpr auto UseVal = true;
#else
    static constexpr auto UseVal = false;
#endif

    static constexpr auto ValLays     = std::array{ "VK_LAYER_KHRONOS_VAL" };
    static constexpr auto ValInstExts = std::array{ VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
    static constexpr auto DevExts     = std::array{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    static constexpr auto                                         MaxFramesInFlight = 2;
    static constexpr size_t                                       DynamicBuffCnt    = 2;
    static constexpr size_t                                       GarbageBuffCnt    = 2;
    //
    // initwindow
    GLFWwindow *                                                  Window;
    bool                                                          FramebufferResized;
    //
    // initInstance
    VkInstance                                                    Inst;
    //
    // initSurface
    VkSurfaceKHR                                                  Surf;
    VkSurfaceFormatKHR                                            SurfFmt;
    //
    // initDebugMessenger
    VkDebugUtilsMessengerEXT                                      DbgMsngr;
    //
    // initDevice
    VkPhysicalDevice                                              PhysicalDev;
    VkDevice                                                      Dev;
    uint32_t                                                      GfxQueueIdx;
    uint32_t                                                      PresentQueueIdx;
    VkQueue                                                       GfxQueue;
    VkQueue                                                       PresentQueue;
    //
    // initLayoutss
    VkDescriptorSetLayout                                         UboDescriptorSetLay;
    VkDescriptorSetLayout                                         TexDescriptorSetLay;
    //
    // PipelineLayouts
    VkPipelineLayout                                              PipelineLay;
    //
    // initPools
    VkCommandPool                                                 CmdPool;
    VkDescriptorPool                                              DescriptorPool;
    //
    // initSwapchain
    uint32_t                                                      SwapchainImgCnt;
    VkSwapchainKHR                                                Swapchain;
    std::vector< VkImageView >                                    SwapchainImgViews;
    VkExtent2D                                                    SwapchainExtent;
    //
    // initDepthImage
    VkImage                                                       DepthImg;
    VkDeviceMemory                                                DepthImgMem;
    VkImageView                                                   DepthImgView;
    //
    // initFramebuffers
    std::vector< VkFramebuffer >                                  Framebuffers;
    //
    // initmain_RenderPass
    VkRenderPass                                                  RdrPass;
    //
    // doesnt belong here
    // ================================================================================================================
    std::vector< unsigned char >                                  texture_;
    uint32_t                                                      width_;
    uint32_t                                                      height_;
    VkImage                                                       image_;
    VkDeviceMemory                                                image_memory_;
    VkImageView                                                   image_view_;
    VkDescriptorSet                                               image_descriptor_set_;
    std::vector< vertex >                                         vertices_;
    std::vector< uint32_t >                                       indices_;
    // ================================================================================================================
    //
    // allocate_commands
    std::array< VkCommandBuffer, DynamicBuffCnt >                 CmdBuffs;
    //
    // shaders
    VkShaderModule                                                VtxShader;
    VkShaderModule                                                FragShader;
    //
    // samplers
    VkSampler                                                     TexSampler;
    //
    // initPipeline
    VkPipeline                                                    Pipeline;
    //
    // initSync
    std::array< VkSemaphore, MaxFramesInFlight >                  ImgAvailableSemaphores;
    std::array< VkSemaphore, MaxFramesInFlight >                  RdrFinishedSemaphores;
    std::array< VkFence, MaxFramesInFlight >                      FrameInFlightFences;
    std::vector< VkFence * >                                      ImgInFlightFences;
    //
    // buffers
    // vertex
    VkMemoryRequirements                                          VtxMemReq;
    VkDeviceSize                                                  VtxAlignedSize;
    std::array< VkBuffer, DynamicBuffCnt >                        VtxBuffs;
    std::array< VkDeviceSize, DynamicBuffCnt >                    VtxOffs;
    VkDeviceMemory                                                VtxMem;
    //
    // index
    VkMemoryRequirements                                          IdxMemReq;
    VkDeviceSize                                                  IdxAlignedSize;
    std::array< VkBuffer, DynamicBuffCnt >                        IdxBuffs;
    std::array< VkDeviceSize, DynamicBuffCnt >                    IdxOffs;
    VkDeviceMemory                                                IdxMem;
    //
    // staging
    VkMemoryRequirements                                          StagingMemReq;
    VkDeviceSize                                                  StagingAlignedSize;
    std::array< VkBuffer, DynamicBuffCnt >                        StagingBuffs;
    std::array< VkDeviceSize, DynamicBuffCnt >                    StagingOffs;
    VkDeviceMemory                                                StagingMem;
    std::byte *                                                   StagingData;
    //
    // ubo
    VkMemoryRequirements                                          UboMemReq;
    VkDeviceSize                                                  UboAlignedSize;
    std::array< VkBuffer, DynamicBuffCnt >                        UboBuffs;
    std::array< uint32_t, DynamicBuffCnt >                        UboOffs;
    std::array< VkDescriptorSet, DynamicBuffCnt >                 UboDescriptorSets;
    VkDeviceMemory                                                UboMem;
    std::byte *                                                   UboData;
    //
    // garbage
    std::array< std::vector< VkBuffer >, GarbageBuffCnt >         GarbageBuffs;
    std::array< std::vector< VkDeviceMemory >, GarbageBuffCnt >   GarbageMems;
    std::array< std::vector< VkDescriptorSet >, GarbageBuffCnt >  GarbageDescriptorSets;
    //
    // rendering counters
    size_t                                                        CurrentFrameIdx   = 0;
    size_t                                                        CurrentBuffIdx    = 0;
    size_t                                                        CurrentGarbageIdx = 0;
    uint32_t                                                      CurrentImgIdx     = 0;
    VkCommandBuffer                                               CurrentCmdBuff    = VK_NULL_HANDLE;
    //
    std::chrono::time_point< std::chrono::high_resolution_clock > StartTime = std::chrono::high_resolution_clock::now();
  };

  [[nodiscard]] float currentTime( Context const & Ctx ) noexcept;

  [[nodiscard]] VkExtent2D queryFramebufferSize( Context const & Ctx ) noexcept;

}  // namespace mvk::engine

#endif
