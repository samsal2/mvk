#include "Engine/Renderer.hpp"

#include <iostream>

namespace Mvk::Engine {

Renderer::Renderer(std::string const &Name, VkExtent2D Extent) noexcept
    : Ctx(std::make_unique<Context>(Name, Extent)) {}

[[nodiscard]] std::unique_ptr<DrawCallGenerator>
Renderer::createDrawCallGenerator() noexcept {
  return std::make_unique<DrawCallGenerator>(*Ctx);
}
void Renderer::preDraw() noexcept {
  Ctx->updateImgIdx({});

  auto const CmdBuff = Ctx->getCurrentCmdBuff();

  auto CmdBuffBeginInfo = VkCommandBufferBeginInfo();
  CmdBuffBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  CmdBuffBeginInfo.flags = 0;
  CmdBuffBeginInfo.pInheritanceInfo = nullptr;

  vkBeginCommandBuffer(CmdBuff, &CmdBuffBeginInfo);
}

void Renderer::beginDraw() noexcept {
  auto const CmdBuff = Ctx->getCurrentCmdBuff();

  auto ClrColorVal = VkClearValue();
  ClrColorVal.color = {{0.0F, 0.0F, 0.0F, 1.0F}};

  auto ClrDepthVal = VkClearValue();
  ClrDepthVal.depthStencil = {1.0F, 0};

  auto const ClrVals = std::array{ClrColorVal, ClrDepthVal};

  auto RenderPassBeginInfo = VkRenderPassBeginInfo();
  RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  RenderPassBeginInfo.renderPass = Ctx->getMainRenderPass();
  RenderPassBeginInfo.framebuffer = Ctx->getCurrentFramebuffer();
  RenderPassBeginInfo.renderArea.offset.x = 0;
  RenderPassBeginInfo.renderArea.offset.y = 0;
  RenderPassBeginInfo.renderArea.extent = Ctx->getSwapchainExtent();
  RenderPassBeginInfo.clearValueCount =
      static_cast<uint32_t>(std::size(ClrVals));
  RenderPassBeginInfo.pClearValues = std::data(ClrVals);

  vkCmdBeginRenderPass(CmdBuff, &RenderPassBeginInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
}

void Renderer::endDraw() noexcept {

  auto const CmdBuff = Ctx->getCurrentCmdBuff();

  vkCmdEndRenderPass(CmdBuff);
  vkEndCommandBuffer(CmdBuff);

  auto const Device = Ctx->getDevice();

  // Wait for the Img in flight to end if it is
  auto const ImgInFlightFence = Ctx->getCurrentImgInFlightFence();

  if (ImgInFlightFence.has_value()) {
    vkWaitForFences(Device, 1, &ImgInFlightFence.value(), VK_TRUE,
                    std::numeric_limits<int64_t>::max());
  }

  Ctx->setCurrentImgInFlightFence(Ctx->getCurrentFrameInFlightFence());

  // get current semaphores
  auto const ImgAvailableSemaphores = Ctx->getCurrentImgAvailableSemaphore();
  auto const RenderFinishedSemaphore = Ctx->getCurrentRenderFinishedSemaphore();

  auto const WaitSemaphore = std::array{ImgAvailableSemaphores};
  auto const SigSemaphore = std::array{RenderFinishedSemaphore};
  auto const WaitStages = std::array<VkPipelineStageFlags, 1>{
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  auto const CmdBuffs = std::array{CmdBuff};

  auto SubmitInfo = VkSubmitInfo();
  SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  SubmitInfo.waitSemaphoreCount =
      static_cast<uint32_t>(std::size(WaitSemaphore));
  SubmitInfo.pWaitSemaphores = std::data(WaitSemaphore);
  SubmitInfo.pWaitDstStageMask = std::data(WaitStages);
  SubmitInfo.commandBufferCount = static_cast<uint32_t>(std::size(CmdBuffs));
  SubmitInfo.pCommandBuffers = std::data(CmdBuffs);
  SubmitInfo.signalSemaphoreCount =
      static_cast<uint32_t>(std::size(SigSemaphore));
  SubmitInfo.pSignalSemaphores = std::data(SigSemaphore);

  auto const FrameInFlightFence = Ctx->getCurrentFrameInFlightFence();
  auto const GfxQueue = Ctx->getGfxQueue();

  vkResetFences(Device, 1, &FrameInFlightFence);
  vkQueueSubmit(GfxQueue, 1, &SubmitInfo, FrameInFlightFence);

  auto const PresentSignalSemaphore = std::array{RenderFinishedSemaphore};
  auto const Swapchains = std::array{Ctx->getSwapchain()};
  auto const ImgIdxs = std::array{Ctx->getCurrentImgIdx()};

  auto PresentInfo = VkPresentInfoKHR();
  PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  PresentInfo.waitSemaphoreCount =
      static_cast<uint32_t>(std::size(PresentSignalSemaphore));
  PresentInfo.pWaitSemaphores = std::data(PresentSignalSemaphore);
  PresentInfo.swapchainCount = static_cast<uint32_t>(std::size(Swapchains));
  PresentInfo.pSwapchains = std::data(Swapchains);
  PresentInfo.pImageIndices = std::data(ImgIdxs);
  PresentInfo.pResults = nullptr;

  auto const PresentQueue = Ctx->getPresentQueue();

  auto Result = vkQueuePresentKHR(PresentQueue, &PresentInfo);
  vkQueueWaitIdle(PresentQueue);

  auto const FramebufferResized = Ctx->isFramebufferResized();
  auto const ChangeSwapchain =
      (Result == VK_ERROR_OUT_OF_DATE_KHR) || (Result == VK_SUBOPTIMAL_KHR);

  if (ChangeSwapchain || FramebufferResized) {
    Ctx->setFramebufferResized(false);
    Ctx->recreateAfterFramebufferChange();
    return;
  }

  MVK_VERIFY(VK_SUCCESS == Result);
  Ctx->updateIdx({});
}

void Renderer::draw(DrawCmd const &Cmd) noexcept {
  auto const CmdBuff = Ctx->getCurrentCmdBuff();

  auto const DescriptorSets =
      std::array{Cmd.Ubo.DescriptorSet, Cmd.Tex.DescriptorSet};
  auto const Pipeline = Ctx->getMainPipeline();
  auto const PipelineLayout = Ctx->getMainPipelineLayout();

  vkCmdBindPipeline(CmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
  vkCmdBindVertexBuffers(CmdBuff, 0, 1, &Cmd.Vtx.Buff, &Cmd.Vtx.Off);
  vkCmdBindIndexBuffer(CmdBuff, Cmd.Idx.Buff, Cmd.Idx.Off,
                       VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(CmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          PipelineLayout, 0,
                          static_cast<uint32_t>(std::size(DescriptorSets)),
                          std::data(DescriptorSets), 1, &Cmd.Ubo.Off);
  vkCmdDrawIndexed(CmdBuff, Cmd.IdxCount, 1, 0, 0, 0);
}

} // namespace Mvk::Engine
