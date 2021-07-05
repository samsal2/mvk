#include "engine/allocate.hpp"

#include "detail/helpers.hpp"
#include "detail/misc.hpp"
#include "utility/verify.hpp"
#include "vulkan/vulkan_core.h"

#include <vulkan/vulkan.h>

namespace mvk::engine
{
  [[nodiscard]] VkCommandBuffer allocSingleUseCmdBuf(In<Context> Ctx) noexcept
  {
    auto Info               = VkCommandBufferAllocateInfo();
    Info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    Info.commandPool        = Ctx->CmdPool;
    Info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    Info.commandBufferCount = 1;

    auto CmdBuf = VkCommandBuffer();

    [[maybe_unused]] auto Result = vkAllocateCommandBuffers(Ctx->Device, &Info, &CmdBuf);
    MVK_VERIFY(Result == VK_SUCCESS);
    return CmdBuf;
  }

  void crtVtxBuffAndMem(InOut<Context> Ctx, VkDeviceSize Size) noexcept
  {
    auto CrtInfo        = VkBufferCreateInfo();
    CrtInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    CrtInfo.size        = Size;
    CrtInfo.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    CrtInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    for (auto & VtxBuff : Ctx->VtxBuffs)
    {
      [[maybe_unused]] auto Result = vkCreateBuffer(Ctx->Device, &CrtInfo, nullptr, &VtxBuff);
      MVK_VERIFY(Result == VK_SUCCESS);
    }

    vkGetBufferMemoryRequirements(Ctx->Device, Ctx->VtxBuffs[Ctx->CurrentBuffIdx], &Ctx->VtxMemReq);
    Ctx->VtxAlignedSize = detail::alignedSize(Ctx->VtxMemReq.size, Ctx->VtxMemReq.alignment);

    auto const MemTypeIdx =
      detail::queryMemType(Ctx->PhysicalDevice, Ctx->VtxMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    MVK_VERIFY(MemTypeIdx.has_value());

    auto VtxMemAllocInfo            = VkMemoryAllocateInfo();
    VtxMemAllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VtxMemAllocInfo.allocationSize  = Context::DynamicBuffCnt * Ctx->VtxAlignedSize;
    VtxMemAllocInfo.memoryTypeIndex = MemTypeIdx.value();

    [[maybe_unused]] auto Result = vkAllocateMemory(Ctx->Device, &VtxMemAllocInfo, nullptr, &Ctx->VtxMem);
    MVK_VERIFY(Result == VK_SUCCESS);

    for (size_t i = 0; i < Context::DynamicBuffCnt; ++i)
    {
      vkBindBufferMemory(Ctx->Device, Ctx->VtxBuffs[i], Ctx->VtxMem, i * Ctx->VtxAlignedSize);
    }
  }

  void crtIdxBuffAndMem(InOut<Context> Ctx, VkDeviceSize size) noexcept
  {
    auto IdxBuffCrtInfo        = VkBufferCreateInfo();
    IdxBuffCrtInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    IdxBuffCrtInfo.size        = size;
    IdxBuffCrtInfo.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    IdxBuffCrtInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    for (auto & IdxBuff : Ctx->IdxBuffs)
    {
      [[maybe_unused]] auto Result = vkCreateBuffer(Ctx->Device, &IdxBuffCrtInfo, nullptr, &IdxBuff);
      MVK_VERIFY(Result == VK_SUCCESS);
    }

    vkGetBufferMemoryRequirements(Ctx->Device, Ctx->IdxBuffs[Ctx->CurrentBuffIdx], &Ctx->IdxMemReq);
    Ctx->IdxAlignedSize = detail::alignedSize(Ctx->IdxMemReq.size, Ctx->IdxMemReq.alignment);

    auto const MemTypeIdx =
      detail::queryMemType(Ctx->PhysicalDevice, Ctx->IdxMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    MVK_VERIFY(MemTypeIdx.has_value());

    auto IdxMemAllocInfo            = VkMemoryAllocateInfo();
    IdxMemAllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    IdxMemAllocInfo.allocationSize  = Context::DynamicBuffCnt * Ctx->IdxAlignedSize;
    IdxMemAllocInfo.memoryTypeIndex = MemTypeIdx.value();

    [[maybe_unused]] auto Result = vkAllocateMemory(Ctx->Device, &IdxMemAllocInfo, nullptr, &Ctx->IdxMem);
    MVK_VERIFY(Result == VK_SUCCESS);

    for (size_t i = 0; i < Context::DynamicBuffCnt; ++i)
    {
      vkBindBufferMemory(Ctx->Device, Ctx->IdxBuffs[i], Ctx->IdxMem, i * Ctx->IdxAlignedSize);
    }
  }

  void crtStagingBuffAndMem(InOut<Context> Ctx, VkDeviceSize size) noexcept
  {
    auto StagingBuffCrtInfo        = VkBufferCreateInfo();
    StagingBuffCrtInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    StagingBuffCrtInfo.size        = size;
    StagingBuffCrtInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    StagingBuffCrtInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    for (auto & StagingBuff : Ctx->StagingBuffs)
    {
      [[maybe_unused]] auto Result = vkCreateBuffer(Ctx->Device, &StagingBuffCrtInfo, nullptr, &StagingBuff);
      MVK_VERIFY(Result == VK_SUCCESS);
    }

    vkGetBufferMemoryRequirements(Ctx->Device, Ctx->StagingBuffs[Ctx->CurrentBuffIdx], &Ctx->StagingMemReq);
    Ctx->StagingAlignedSize = detail::alignedSize(Ctx->StagingMemReq.size, Ctx->StagingMemReq.alignment);

    auto const MemTypeIdx =
      detail::queryMemType(Ctx->PhysicalDevice,
                           Ctx->StagingMemReq.memoryTypeBits,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    MVK_VERIFY(MemTypeIdx.value());

    auto StagingMemAllocInfo            = VkMemoryAllocateInfo();
    StagingMemAllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    StagingMemAllocInfo.allocationSize  = Context::DynamicBuffCnt * Ctx->StagingAlignedSize;
    StagingMemAllocInfo.memoryTypeIndex = MemTypeIdx.value();

    [[maybe_unused]] auto Result = vkAllocateMemory(Ctx->Device, &StagingMemAllocInfo, nullptr, &Ctx->StagingMem);
    MVK_VERIFY(Result == VK_SUCCESS);

    void * Data = nullptr;

    Result = vkMapMemory(Ctx->Device, Ctx->StagingMem, 0, VK_WHOLE_SIZE, 0, &Data);
    MVK_VERIFY(Result == VK_SUCCESS);

    Ctx->StagingData = static_cast<std::byte *>(Data);

    for (size_t i = 0; i < Context::DynamicBuffCnt; ++i)
    {
      vkBindBufferMemory(Ctx->Device, Ctx->StagingBuffs[i], Ctx->StagingMem, i * Ctx->StagingAlignedSize);
    }
  }

  void mvToGarbageBuff(InOut<Context> Ctx, utility::Slice<VkBuffer> Buffs) noexcept
  {
    auto & GarbageBuffers = Ctx->GarbageBuffs[Ctx->CurrentGarbageIdx];
    GarbageBuffers.reserve(std::size(GarbageBuffers) + std::size(Buffs));

    for (auto const Buff : Buffs)
    {
      Ctx->GarbageBuffs[Ctx->CurrentGarbageIdx].push_back(Buff);
    }
  }

  void mvToGarbageSets(InOut<Context> Ctx, utility::Slice<VkDescriptorSet> Sets) noexcept
  {
    auto & GarbageDescriptorSets = Ctx->GarbageDescriptorSets[Ctx->CurrentGarbageIdx];
    GarbageDescriptorSets.reserve(std::size(GarbageDescriptorSets) + std::size(Sets));

    for (auto const Set : Sets)
    {
      Ctx->GarbageDescriptorSets[Ctx->CurrentGarbageIdx].push_back(Set);
    }
  }

  void mvToGarbageMem(InOut<Context> Ctx, VkDeviceMemory Mem) noexcept
  {
    Ctx->GarbageMems[Ctx->CurrentGarbageIdx].push_back(Mem);
  }

  [[nodiscard]] StagingAlloc allocStaging(InOut<Context> Ctx, utility::Slice<std::byte const> Src) noexcept
  {
    Ctx->StagingOffs[Ctx->CurrentBuffIdx] =
      detail::alignedSize(Ctx->StagingOffs[Ctx->CurrentBuffIdx], Ctx->StagingMemReq.alignment);

    if (auto ReqSize = Ctx->StagingOffs[Ctx->CurrentBuffIdx] + std::size(Src); ReqSize > Ctx->StagingAlignedSize)
    {
      mvToGarbageBuff(Ctx, Ctx->StagingBuffs);
      mvToGarbageMem(Ctx, Ctx->StagingMem);
      crtStagingBuffAndMem(Ctx, ReqSize * 2);
      Ctx->StagingOffs[Ctx->CurrentBuffIdx] = 0;
    }

    auto const BuffOff = Ctx->StagingOffs[Ctx->CurrentBuffIdx];

    auto const MemOff = Ctx->CurrentBuffIdx * Ctx->StagingAlignedSize + BuffOff;
    std::copy(std::begin(Src), std::end(Src), Ctx->StagingData + MemOff);
    Ctx->StagingOffs[Ctx->CurrentBuffIdx] += std::size(Src);

    return { Ctx->StagingBuffs[Ctx->CurrentBuffIdx], BuffOff, std::size(Src) };
  }

  [[nodiscard]] VtxAlloc allocVtx(InOut<Context> Ctx, StagingAlloc Alloc) noexcept
  {
    if (auto ReqSize = Ctx->VtxOffs[Ctx->CurrentBuffIdx] + Alloc.Size; ReqSize > Ctx->VtxAlignedSize)
    {
      mvToGarbageBuff(Ctx, Ctx->VtxBuffs);
      mvToGarbageMem(Ctx, Ctx->VtxMem);
      crtVtxBuffAndMem(Ctx, ReqSize * 2);
      Ctx->VtxOffs[Ctx->CurrentBuffIdx] = 0;
    }

    auto const VtxOff  = Ctx->VtxOffs[Ctx->CurrentBuffIdx];
    auto const VtxBuff = Ctx->VtxBuffs[Ctx->CurrentBuffIdx];

    auto CmdBuffBeginInfo             = VkCommandBufferBeginInfo();
    CmdBuffBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CmdBuffBeginInfo.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CmdBuffBeginInfo.pInheritanceInfo = nullptr;

    auto CopyRegion      = VkBufferCopy();
    CopyRegion.srcOffset = Alloc.Off;
    CopyRegion.dstOffset = VtxOff;
    CopyRegion.size      = Alloc.Size;

    auto const CmdBuf = allocSingleUseCmdBuf(Ctx);
    vkBeginCommandBuffer(CmdBuf, &CmdBuffBeginInfo);

    vkCmdCopyBuffer(CmdBuf, Alloc.Buff, VtxBuff, 1, &CopyRegion);

    vkEndCommandBuffer(CmdBuf);

    auto SubmitInfo               = VkSubmitInfo();
    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers    = &CmdBuf;

    vkQueueSubmit(Ctx->GfxQueue, 1, &SubmitInfo, nullptr);
    vkQueueWaitIdle(Ctx->GfxQueue);
    vkFreeCommandBuffers(Ctx->Device, Ctx->CmdPool, 1, &CmdBuf);

    Ctx->VtxOffs[Ctx->CurrentBuffIdx] += Alloc.Size;

    return { VtxBuff, VtxOff };
  }

  [[nodiscard]] IdxAlloc allocIdx(InOut<Context> Ctx, StagingAlloc Alloc) noexcept
  {
    if (auto ReqSize = Ctx->IdxOffs[Ctx->CurrentBuffIdx] + Alloc.Size; ReqSize > Ctx->IdxAlignedSize)
    {
      mvToGarbageBuff(Ctx, Ctx->IdxBuffs);
      mvToGarbageMem(Ctx, Ctx->IdxMem);
      crtIdxBuffAndMem(Ctx, ReqSize * 2);
      Ctx->IdxOffs[Ctx->CurrentBuffIdx] = 0;
    }

    auto const IdxOff  = Ctx->IdxOffs[Ctx->CurrentBuffIdx];
    auto const IdxBuff = Ctx->IdxBuffs[Ctx->CurrentBuffIdx];

    auto CmdBuffBeginInfo             = VkCommandBufferBeginInfo();
    CmdBuffBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CmdBuffBeginInfo.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CmdBuffBeginInfo.pInheritanceInfo = nullptr;

    auto CopyRegion      = VkBufferCopy();
    CopyRegion.srcOffset = Alloc.Off;
    CopyRegion.dstOffset = IdxOff;
    CopyRegion.size      = Alloc.Size;

    auto const CmdBuf = allocSingleUseCmdBuf(Ctx);
    vkBeginCommandBuffer(CmdBuf, &CmdBuffBeginInfo);

    vkCmdCopyBuffer(CmdBuf, Alloc.Buff, IdxBuff, 1, &CopyRegion);

    vkEndCommandBuffer(CmdBuf);

    auto SubmitInfo               = VkSubmitInfo();
    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers    = &CmdBuf;

    vkQueueSubmit(Ctx->GfxQueue, 1, &SubmitInfo, nullptr);
    vkQueueWaitIdle(Ctx->GfxQueue);
    vkFreeCommandBuffers(Ctx->Device, Ctx->CmdPool, 1, &CmdBuf);

    Ctx->IdxOffs[Ctx->CurrentBuffIdx] += Alloc.Size;

    return { IdxBuff, IdxOff };
  }

  void crtStagingBuffMemAndSets(InOut<Context> Ctx, VkDeviceSize Size) noexcept
  {
    auto UboBuffCrtInfo        = VkBufferCreateInfo();
    UboBuffCrtInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    UboBuffCrtInfo.size        = Size;
    UboBuffCrtInfo.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    UboBuffCrtInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    for (auto & UboBuff : Ctx->UboBuffs)
    {
      [[maybe_unused]] auto Result = vkCreateBuffer(Ctx->Device, &UboBuffCrtInfo, nullptr, &UboBuff);
      MVK_VERIFY(Result == VK_SUCCESS);
    }

    vkGetBufferMemoryRequirements(Ctx->Device, Ctx->UboBuffs[Ctx->CurrentBuffIdx], &Ctx->UboMemReq);

    Ctx->UboAlignedSize = detail::alignedSize(Ctx->UboMemReq.size, Ctx->UboMemReq.alignment);

    auto const MemTypeIdx =
      detail::queryMemType(Ctx->PhysicalDevice,
                           Ctx->UboMemReq.memoryTypeBits,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    MVK_VERIFY(MemTypeIdx.value());

    auto UboMemAllocInfo            = VkMemoryAllocateInfo();
    UboMemAllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    UboMemAllocInfo.allocationSize  = Context::DynamicBuffCnt * Ctx->UboAlignedSize;
    UboMemAllocInfo.memoryTypeIndex = MemTypeIdx.value();

    [[maybe_unused]] auto Result = vkAllocateMemory(Ctx->Device, &UboMemAllocInfo, nullptr, &Ctx->UboMem);
    MVK_VERIFY(Result == VK_SUCCESS);

    void * Data  = nullptr;
    Result       = vkMapMemory(Ctx->Device, Ctx->UboMem, 0, VK_WHOLE_SIZE, 0, &Data);
    Ctx->UboData = static_cast<std::byte *>(Data);
    MVK_VERIFY(Result == VK_SUCCESS);

    Ctx->UboDescriptorSets = allocDescriptorSets<Context::DynamicBuffCnt>(Ctx, Ctx->UboDescriptorSetLayout);

    for (size_t i = 0; i < Context::DynamicBuffCnt; ++i)
    {
      vkBindBufferMemory(Ctx->Device, Ctx->UboBuffs[i], Ctx->UboMem, i * Ctx->UboAlignedSize);

      auto UboDescriptorBuffInfo   = VkDescriptorBufferInfo();
      UboDescriptorBuffInfo.buffer = Ctx->UboBuffs[i];
      UboDescriptorBuffInfo.offset = 0;
      UboDescriptorBuffInfo.range  = sizeof(pvm);

      auto UboWriteDescriptorSet             = VkWriteDescriptorSet();
      UboWriteDescriptorSet.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      UboWriteDescriptorSet.dstSet           = Ctx->UboDescriptorSets[i];
      UboWriteDescriptorSet.dstBinding       = 0;
      UboWriteDescriptorSet.dstArrayElement  = 0;
      UboWriteDescriptorSet.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
      UboWriteDescriptorSet.descriptorCount  = 1;
      UboWriteDescriptorSet.pBufferInfo      = &UboDescriptorBuffInfo;
      UboWriteDescriptorSet.pImageInfo       = nullptr;
      UboWriteDescriptorSet.pTexelBufferView = nullptr;

      vkUpdateDescriptorSets(Ctx->Device, 1, &UboWriteDescriptorSet, 0, nullptr);
    }
  }

  [[nodiscard]] UboAlloc allocUbo(InOut<Context> Ctx, utility::Slice<std::byte const> src) noexcept
  {
    Ctx->UboOffs[Ctx->CurrentBuffIdx] =
      detail::alignedSize(Ctx->UboOffs[Ctx->CurrentBuffIdx], static_cast<uint32_t>(Ctx->UboMemReq.alignment));

    if (auto ReqSize = Ctx->UboOffs[Ctx->CurrentBuffIdx] + std::size(src); ReqSize > Ctx->UboAlignedSize)
    {
      mvToGarbageBuff(Ctx, Ctx->UboBuffs);
      mvToGarbageSets(Ctx, Ctx->UboDescriptorSets);
      mvToGarbageMem(Ctx, Ctx->UboMem);
      crtStagingBuffMemAndSets(Ctx, ReqSize * 2);
      Ctx->UboOffs[Ctx->CurrentBuffIdx] = 0;
    }
    auto const BuffOff = Ctx->UboOffs[Ctx->CurrentBuffIdx];
    auto const MemOff  = Ctx->CurrentBuffIdx * Ctx->UboAlignedSize + BuffOff;
    std::copy(std::begin(src), std::end(src), Ctx->UboData + MemOff);
    Ctx->UboOffs[Ctx->CurrentBuffIdx] += std::size(src);

    return { Ctx->UboDescriptorSets[Ctx->CurrentBuffIdx], BuffOff };
  }

  void dtyVtxBuffAndMem(InOut<Context> Ctx) noexcept
  {
    vkFreeMemory(Ctx->Device, Ctx->VtxMem, nullptr);
    for (auto const VtxBuff : Ctx->VtxBuffs)
    {
      vkDestroyBuffer(Ctx->Device, VtxBuff, nullptr);
    }
  }

  void dtyIdxBuffAndMem(InOut<Context> Ctx) noexcept
  {
    vkFreeMemory(Ctx->Device, Ctx->IdxMem, nullptr);
    for (auto const IdxBuff : Ctx->IdxBuffs)
    {
      vkDestroyBuffer(Ctx->Device, IdxBuff, nullptr);
    }
  }

  void dtyStagingBuffAndMem(InOut<Context> Ctx) noexcept
  {
    vkFreeMemory(Ctx->Device, Ctx->StagingMem, nullptr);
    for (auto const StagingBuff : Ctx->StagingBuffs)
    {
      vkDestroyBuffer(Ctx->Device, StagingBuff, nullptr);
    }
  }

  void dtyBuffMemAndSet(InOut<Context> Ctx) noexcept
  {
    vkFreeDescriptorSets(Ctx->Device,
                         Ctx->DescriptorPool,
                         static_cast<uint32_t>(std::size(Ctx->UboDescriptorSets)),
                         std::data(Ctx->UboDescriptorSets));

    vkFreeMemory(Ctx->Device, Ctx->UboMem, nullptr);
    for (auto const UboBuff : Ctx->UboBuffs)
    {
      vkDestroyBuffer(Ctx->Device, UboBuff, nullptr);
    }
  }

  void dtyGarbageBuff(InOut<Context> Ctx) noexcept
  {
    for (auto const & GarbageBuffs : Ctx->GarbageBuffs)
    {
      for (auto const GarbageBuff : GarbageBuffs)
      {
        vkDestroyBuffer(Ctx->Device, GarbageBuff, nullptr);
      }
    }
  }

  void dtyGarbageMem(InOut<Context> Ctx) noexcept
  {
    {
      for (auto const & GarbageMems : Ctx->GarbageMems)
      {
        for (auto const GarbageMem : GarbageMems)
        {
          vkFreeMemory(Ctx->Device, GarbageMem, nullptr);
        }
      }
    }
  }
  void dtyGarbageSets(InOut<Context> Ctx) noexcept
  {
    for (auto const & GarbageDescriptorSets : Ctx->GarbageDescriptorSets)
    {
      for (auto const GarbageDescriptorSet : GarbageDescriptorSets)
      {
        vkFreeDescriptorSets(Ctx->Device, Ctx->DescriptorPool, 1, &GarbageDescriptorSet);
      }
    }
  }

  void nextBuffer(InOut<Context> Ctx) noexcept
  {
    Ctx->CurrentBuffIdx = (Ctx->CurrentBuffIdx + 1) % Context::DynamicBuffCnt;

    Ctx->StagingOffs[Ctx->CurrentBuffIdx] = 0;
    Ctx->VtxOffs[Ctx->CurrentBuffIdx]     = 0;
    Ctx->IdxOffs[Ctx->CurrentBuffIdx]     = 0;
    Ctx->UboOffs[Ctx->CurrentBuffIdx]     = 0;
  }

}  // namespace mvk::engine
