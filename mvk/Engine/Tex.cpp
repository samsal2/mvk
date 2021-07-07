#include "Engine/Tex.hpp"

#include "Detail/Misc.hpp"

namespace Mvk::Engine {

Tex::Tex(Context &Ctx, StagingMgr::MapResult From, uint32_t Width,
         uint32_t Height) noexcept
    : State(AllocState::Deallocated), Ctx(Ctx), Width(Width), Height(Height),
      Img(VK_NULL_HANDLE), ImgView(VK_NULL_HANDLE), Mem(VK_NULL_HANDLE),
      DescriptorSet(VK_NULL_HANDLE) {
  allocate(From);
  write();
}

Tex::~Tex() noexcept {
  if (State == AllocState::Allocated)
    deallocate();
}

void Tex::allocate(StagingMgr::MapResult From) noexcept {
  MVK_VERIFY(State == AllocState::Deallocated);

  MipLvl = Detail::calcMipLvl(Width, Height);

  auto ImgCrtInfo = VkImageCreateInfo();
  ImgCrtInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ImgCrtInfo.imageType = VK_IMAGE_TYPE_2D;
  ImgCrtInfo.extent.width = Width;
  ImgCrtInfo.extent.height = Height;
  ImgCrtInfo.extent.depth = 1;
  ImgCrtInfo.mipLevels = MipLvl;
  ImgCrtInfo.arrayLayers = 1;
  ImgCrtInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
  ImgCrtInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  ImgCrtInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  ImgCrtInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                     VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                     VK_IMAGE_USAGE_SAMPLED_BIT;
  ImgCrtInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  ImgCrtInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  ImgCrtInfo.flags = 0;

  auto const Device = Ctx.getDevice();

  auto Result = vkCreateImage(Device, &ImgCrtInfo, nullptr, &Img);
  MVK_VERIFY(Result == VK_SUCCESS);

  auto ImgMemReq = VkMemoryRequirements();
  vkGetImageMemoryRequirements(Device, Img, &ImgMemReq);

  auto const PhysicalDevice = Ctx.getPhysicalDevice();

  auto const MemTypeIdx =
      Detail::queryMemType(PhysicalDevice, ImgMemReq.memoryTypeBits,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  MVK_VERIFY(MemTypeIdx.has_value());

  auto ImgMemAllocInfo = VkMemoryAllocateInfo();
  ImgMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  ImgMemAllocInfo.allocationSize = ImgMemReq.size;
  ImgMemAllocInfo.memoryTypeIndex = MemTypeIdx.value();

  Result = vkAllocateMemory(Device, &ImgMemAllocInfo, nullptr, &Mem);
  MVK_VERIFY(Result == VK_SUCCESS);

  vkBindImageMemory(Device, Img, Mem, 0);

  auto CmdBuff = Ctx.getCurrentCmdBuff();

  transitionImgLayout(CmdBuff, Img, VK_IMAGE_LAYOUT_UNDEFINED,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      ImgCrtInfo.mipLevels);
  stage(From);
  generateMip();

  auto ImgViewCrtInfo = VkImageViewCreateInfo();
  ImgViewCrtInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ImgViewCrtInfo.image = Img;
  ImgViewCrtInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  ImgViewCrtInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
  ImgViewCrtInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  ImgViewCrtInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  ImgViewCrtInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  ImgViewCrtInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  ImgViewCrtInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  ImgViewCrtInfo.subresourceRange.baseMipLevel = 0;
  ImgViewCrtInfo.subresourceRange.levelCount = ImgCrtInfo.mipLevels;
  ImgViewCrtInfo.subresourceRange.baseArrayLayer = 0;
  ImgViewCrtInfo.subresourceRange.layerCount = 1;

  Result = vkCreateImageView(Device, &ImgViewCrtInfo, nullptr, &ImgView);
  MVK_VERIFY(Result == VK_SUCCESS);

  auto const DescriptorPool = Ctx.getDescPool();
  auto const TexDescriptorSetLayout = Ctx.getTexDescriptorSetLayout();

  auto DescriptorSetAllocInfo = VkDescriptorSetAllocateInfo();
  DescriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  DescriptorSetAllocInfo.descriptorPool = DescriptorPool;
  DescriptorSetAllocInfo.descriptorSetCount = 1;
  DescriptorSetAllocInfo.pSetLayouts = &TexDescriptorSetLayout;

  Result =
      vkAllocateDescriptorSets(Device, &DescriptorSetAllocInfo, &DescriptorSet);
  MVK_VERIFY(Result == VK_SUCCESS);

  State = AllocState::Allocated;
}

void Tex::deallocate() noexcept {
  MVK_VERIFY(State == AllocState::Allocated);

  auto const Device = Ctx.getDevice();
  auto const DescriptorPool = Ctx.getDescPool();

  vkFreeDescriptorSets(Device, DescriptorPool, 1, &DescriptorSet);
  vkDestroyImageView(Device, ImgView, nullptr);
  vkFreeMemory(Device, Mem, nullptr);
  vkDestroyImage(Device, Img, nullptr);

  State = AllocState::Deallocated;
}

void Tex::write() noexcept {
  auto ImgDescriptorImgInfo = VkDescriptorImageInfo();
  ImgDescriptorImgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  ImgDescriptorImgInfo.imageView = ImgView;
  ImgDescriptorImgInfo.sampler = Ctx.getTexSampler();

  auto ImgWriteDescriptorSet = VkWriteDescriptorSet();
  ImgWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  ImgWriteDescriptorSet.dstSet = DescriptorSet;
  ImgWriteDescriptorSet.dstBinding = 0;
  ImgWriteDescriptorSet.dstArrayElement = 0;
  ImgWriteDescriptorSet.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  ImgWriteDescriptorSet.descriptorCount = 1;
  ImgWriteDescriptorSet.pBufferInfo = nullptr;
  ImgWriteDescriptorSet.pImageInfo = &ImgDescriptorImgInfo;
  ImgWriteDescriptorSet.pTexelBufferView = nullptr;

  auto const Device = Ctx.getDevice();
  vkUpdateDescriptorSets(Device, 1, &ImgWriteDescriptorSet, 0, nullptr);
}

void Tex::transitionImgLayout(VkCommandBuffer CmdBuff, VkImage Img,
                              VkImageLayout OldLay, VkImageLayout NewLay,
                              uint32_t MipLvl) noexcept {

  auto ImgMemBarrier = VkImageMemoryBarrier();
  ImgMemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  ImgMemBarrier.oldLayout = OldLay;
  ImgMemBarrier.newLayout = NewLay;
  ImgMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  ImgMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  ImgMemBarrier.image = Img;

  if (NewLay == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    ImgMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  } else {
    ImgMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  ImgMemBarrier.subresourceRange.baseMipLevel = 0;
  ImgMemBarrier.subresourceRange.levelCount = MipLvl;
  ImgMemBarrier.subresourceRange.baseArrayLayer = 0;
  ImgMemBarrier.subresourceRange.layerCount = 1;

  auto SrcStage = VkPipelineStageFlags();
  auto DstStage = VkPipelineStageFlags();

  if (OldLay == VK_IMAGE_LAYOUT_UNDEFINED &&
      NewLay == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    ImgMemBarrier.srcAccessMask = 0;
    ImgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    DstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (OldLay == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             NewLay == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    ImgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    ImgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    DstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (OldLay == VK_IMAGE_LAYOUT_UNDEFINED &&
             NewLay == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    ImgMemBarrier.srcAccessMask = 0;
    ImgMemBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    DstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else {
    MVK_VERIFY_NOT_REACHED();
  }

  vkCmdPipelineBarrier(CmdBuff, SrcStage, DstStage, 0, 0, nullptr, 0, nullptr,
                       1, &ImgMemBarrier);
}

void Tex::generateMip() noexcept {

  if (MipLvl == 1 || MipLvl == 0) {
    return;
  }

  auto ImgMemBarrier = VkImageMemoryBarrier();
  ImgMemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  ImgMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  ImgMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  ImgMemBarrier.image = Img;
  ImgMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  ImgMemBarrier.subresourceRange.baseArrayLayer = 0;
  ImgMemBarrier.subresourceRange.layerCount = 1;
  ImgMemBarrier.subresourceRange.levelCount = 1;

  auto MipWidth = static_cast<int32_t>(Width);
  auto MipHeight = static_cast<int32_t>(Height);

  auto const half = [](auto &num) {
    if (num > 1) {
      num /= 2;
      return num;
    }

    return 1;
  };

  auto const CmdBuff = Ctx.getCurrentCmdBuff();

  for (auto i = uint32_t(0); i < (MipLvl - 1); ++i) {

    ImgMemBarrier.subresourceRange.baseMipLevel = i;
    ImgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    ImgMemBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    ImgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    ImgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(CmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &ImgMemBarrier);

    auto ImgBlit = VkImageBlit();
    ImgBlit.srcOffsets[0].x = 0;
    ImgBlit.srcOffsets[0].y = 0;
    ImgBlit.srcOffsets[0].z = 0;
    ImgBlit.srcOffsets[1].x = MipWidth;
    ImgBlit.srcOffsets[1].y = MipHeight;
    ImgBlit.srcOffsets[1].z = 1;
    ImgBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ImgBlit.srcSubresource.mipLevel = i;
    ImgBlit.srcSubresource.baseArrayLayer = 0;
    ImgBlit.srcSubresource.layerCount = 1;
    ImgBlit.dstOffsets[0].x = 0;
    ImgBlit.dstOffsets[0].y = 0;
    ImgBlit.dstOffsets[0].z = 0;
    ImgBlit.dstOffsets[1].x = half(MipWidth);
    ImgBlit.dstOffsets[1].y = half(MipHeight);
    ImgBlit.dstOffsets[1].z = 1;
    ImgBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ImgBlit.dstSubresource.mipLevel = i + 1;
    ImgBlit.dstSubresource.baseArrayLayer = 0;
    ImgBlit.dstSubresource.layerCount = 1;

    vkCmdBlitImage(CmdBuff, Img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Img,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImgBlit,
                   VK_FILTER_LINEAR);

    ImgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    ImgMemBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ImgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    ImgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(CmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &ImgMemBarrier);
  }

  ImgMemBarrier.subresourceRange.baseMipLevel = MipLvl - 1;
  ImgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  ImgMemBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  ImgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  ImgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(CmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &ImgMemBarrier);
}

void Tex::stage(StagingMgr::MapResult From) noexcept {

  MVK_VERIFY(From.Size == size());

  auto CopyRegion = VkBufferImageCopy();
  CopyRegion.bufferOffset = From.Off;
  CopyRegion.bufferRowLength = 0;
  CopyRegion.bufferImageHeight = 0;
  CopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  CopyRegion.imageSubresource.mipLevel = 0;
  CopyRegion.imageSubresource.baseArrayLayer = 0;
  CopyRegion.imageSubresource.layerCount = 1;
  CopyRegion.imageOffset.x = 0;
  CopyRegion.imageOffset.y = 0;
  CopyRegion.imageOffset.z = 0;
  CopyRegion.imageExtent.width = Width;
  CopyRegion.imageExtent.height = Height;
  CopyRegion.imageExtent.depth = 1;

  auto const CmdBuff = Ctx.getCurrentCmdBuff();
  vkCmdCopyBufferToImage(CmdBuff, From.Buff, Img,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &CopyRegion);
}

} // namespace Mvk::Engine