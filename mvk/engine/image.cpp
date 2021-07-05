#include "engine/image.hpp"

#include "vulkan/vulkan_core.h"

namespace mvk::engine
{
  void trainstionLay(In<Context> Ctx, VkImage Img, VkImageLayout OldLay, VkImageLayout NewLay, uint32_t MipLvl) noexcept
  {
    auto ImgMemBarrier                = VkImageMemoryBarrier();
    ImgMemBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    ImgMemBarrier.oldLayout           = OldLay;
    ImgMemBarrier.newLayout           = NewLay;
    ImgMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ImgMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ImgMemBarrier.image               = Img;

    if (NewLay == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
      ImgMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else
    {
      ImgMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    ImgMemBarrier.subresourceRange.baseMipLevel   = 0;
    ImgMemBarrier.subresourceRange.levelCount     = MipLvl;
    ImgMemBarrier.subresourceRange.baseArrayLayer = 0;
    ImgMemBarrier.subresourceRange.layerCount     = 1;

    auto SrcStage = VkPipelineStageFlags();
    auto DstStage = VkPipelineStageFlags();

    if (OldLay == VK_IMAGE_LAYOUT_UNDEFINED && NewLay == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
      ImgMemBarrier.srcAccessMask = 0;
      ImgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      DstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (OldLay == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLay == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
      ImgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      ImgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      DstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (OldLay == VK_IMAGE_LAYOUT_UNDEFINED && NewLay == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
      ImgMemBarrier.srcAccessMask = 0;
      ImgMemBarrier.dstAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      DstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
      MVK_VERIFY_NOT_REACHED();
    }

    auto CmdBuffBeginInfo             = VkCommandBufferBeginInfo();
    CmdBuffBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CmdBuffBeginInfo.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CmdBuffBeginInfo.pInheritanceInfo = nullptr;

    auto const CmdBuff = allocSingleUseCmdBuf(Ctx);

    vkBeginCommandBuffer(CmdBuff, &CmdBuffBeginInfo);
    vkCmdPipelineBarrier(CmdBuff, SrcStage, DstStage, 0, 0, nullptr, 0, nullptr, 1, &ImgMemBarrier);

    vkEndCommandBuffer(CmdBuff);

    auto SubmitInfo               = VkSubmitInfo();
    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers    = &CmdBuff;

    vkQueueSubmit(Ctx->GfxQueue, 1, &SubmitInfo, nullptr);
    vkQueueWaitIdle(Ctx->GfxQueue);
    vkFreeCommandBuffers(Ctx->Dev, Ctx->CmdPool, 1, &CmdBuff);
  }

  void generateMip(In<Context> Ctx, VkImage Img, uint32_t Width, uint32_t Height, uint32_t MipLvl) noexcept
  {
    if (MipLvl == 1 || MipLvl == 0)
    {
      return;
    }

    auto CmdBuffBeginInfo             = VkCommandBufferBeginInfo();
    CmdBuffBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CmdBuffBeginInfo.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CmdBuffBeginInfo.pInheritanceInfo = nullptr;

    auto CmdBuff = allocSingleUseCmdBuf(Ctx);
    vkBeginCommandBuffer(CmdBuff, &CmdBuffBeginInfo);

    auto ImgMemBarrier                            = VkImageMemoryBarrier();
    ImgMemBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    ImgMemBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    ImgMemBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    ImgMemBarrier.image                           = Img;
    ImgMemBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    ImgMemBarrier.subresourceRange.baseArrayLayer = 0;
    ImgMemBarrier.subresourceRange.layerCount     = 1;
    ImgMemBarrier.subresourceRange.levelCount     = 1;

    auto MipWidth  = static_cast<int32_t>(Width);
    auto MipHeight = static_cast<int32_t>(Height);

    auto const half = [](auto & num)
    {
      if (num > 1)
      {
        num /= 2;
        return num;
      }

      return 1;
    };

    for (auto i = uint32_t(0); i < (MipLvl - 1); ++i)
    {
      ImgMemBarrier.subresourceRange.baseMipLevel = i;
      ImgMemBarrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      ImgMemBarrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      ImgMemBarrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
      ImgMemBarrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

      vkCmdPipelineBarrier(CmdBuff,
                           VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_TRANSFER_BIT,
                           0,
                           0,
                           nullptr,
                           0,
                           nullptr,
                           1,
                           &ImgMemBarrier);

      auto ImgBlit                          = VkImageBlit();
      ImgBlit.srcOffsets[0].x               = 0;
      ImgBlit.srcOffsets[0].y               = 0;
      ImgBlit.srcOffsets[0].z               = 0;
      ImgBlit.srcOffsets[1].x               = MipWidth;
      ImgBlit.srcOffsets[1].y               = MipHeight;
      ImgBlit.srcOffsets[1].z               = 1;
      ImgBlit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      ImgBlit.srcSubresource.mipLevel       = i;
      ImgBlit.srcSubresource.baseArrayLayer = 0;
      ImgBlit.srcSubresource.layerCount     = 1;
      ImgBlit.dstOffsets[0].x               = 0;
      ImgBlit.dstOffsets[0].y               = 0;
      ImgBlit.dstOffsets[0].z               = 0;
      ImgBlit.dstOffsets[1].x               = half(MipWidth);
      ImgBlit.dstOffsets[1].y               = half(MipHeight);
      ImgBlit.dstOffsets[1].z               = 1;
      ImgBlit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      ImgBlit.dstSubresource.mipLevel       = i + 1;
      ImgBlit.dstSubresource.baseArrayLayer = 0;
      ImgBlit.dstSubresource.layerCount     = 1;

      vkCmdBlitImage(CmdBuff,
                     Img,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     Img,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     1,
                     &ImgBlit,
                     VK_FILTER_LINEAR);

      ImgMemBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      ImgMemBarrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      ImgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      ImgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      vkCmdPipelineBarrier(CmdBuff,
                           VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                           0,
                           0,
                           nullptr,
                           0,
                           nullptr,
                           1,
                           &ImgMemBarrier);
    }

    ImgMemBarrier.subresourceRange.baseMipLevel = MipLvl - 1;
    ImgMemBarrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    ImgMemBarrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ImgMemBarrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
    ImgMemBarrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(CmdBuff,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &ImgMemBarrier);

    vkEndCommandBuffer(CmdBuff);

    auto SubmitInfo               = VkSubmitInfo();
    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers    = &CmdBuff;

    vkQueueSubmit(Ctx->GfxQueue, 1, &SubmitInfo, nullptr);
    vkQueueWaitIdle(Ctx->GfxQueue);
    vkFreeCommandBuffers(Ctx->Dev, Ctx->CmdPool, 1, &CmdBuff);
  }

  // buffers

  void stageImage(In<Context> Ctx, StagingAlloc Alloc, uint32_t Width, uint32_t Height, VkImage Img) noexcept
  {
    auto CmdBuffBeginInfo             = VkCommandBufferBeginInfo();
    CmdBuffBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CmdBuffBeginInfo.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CmdBuffBeginInfo.pInheritanceInfo = nullptr;

    auto CopyRegion                            = VkBufferImageCopy();
    CopyRegion.bufferOffset                    = Alloc.Off;
    CopyRegion.bufferRowLength                 = 0;
    CopyRegion.bufferImageHeight               = 0;
    CopyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    CopyRegion.imageSubresource.mipLevel       = 0;
    CopyRegion.imageSubresource.baseArrayLayer = 0;
    CopyRegion.imageSubresource.layerCount     = 1;
    CopyRegion.imageOffset.x                   = 0;
    CopyRegion.imageOffset.y                   = 0;
    CopyRegion.imageOffset.z                   = 0;
    CopyRegion.imageExtent.width               = Width;
    CopyRegion.imageExtent.height              = Height;
    CopyRegion.imageExtent.depth               = 1;

    auto const CmdBuff = allocSingleUseCmdBuf(Ctx);

    vkBeginCommandBuffer(CmdBuff, &CmdBuffBeginInfo);

    vkCmdCopyBufferToImage(CmdBuff, Alloc.Buff, Img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &CopyRegion);

    vkEndCommandBuffer(CmdBuff);

    auto SubmitInfo               = VkSubmitInfo();
    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers    = &CmdBuff;

    vkQueueSubmit(Ctx->GfxQueue, 1, &SubmitInfo, nullptr);
    vkQueueWaitIdle(Ctx->GfxQueue);
    vkFreeCommandBuffers(Ctx->Dev, Ctx->CmdPool, 1, &CmdBuff);
  }

}  // namespace mvk::engine
