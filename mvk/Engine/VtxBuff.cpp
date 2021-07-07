#include "Engine/VtxBuff.hpp"

#include "Detail/Misc.hpp"
#include "Utility/Verify.hpp"
#include "vulkan/vulkan_core.h"

namespace Mvk::Engine {

VtxBuff::VtxBuff(Context &Ctx, VkDeviceSize Size) noexcept
    : State(AllocState::Deallocated), Ctx(Ctx), MemReq(), LastReqSize(0),
      AlignedSize(0), Buffs(), Offs(), Mem(VK_NULL_HANDLE), BuffIdx(0) {
  allocate(Size);
}

VtxBuff::~VtxBuff() noexcept {
  if (State == AllocState::Allocated)
    deallocate();
}

void VtxBuff::allocate(VkDeviceSize Size) noexcept {
  MVK_VERIFY(State == AllocState::Deallocated);

  LastReqSize = Size;

  auto CrtInfo = VkBufferCreateInfo();
  CrtInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  CrtInfo.size = Size;
  CrtInfo.usage =
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  CrtInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  auto const Device = Ctx.getDevice();

  for (auto &VtxBuff : Buffs) {
    auto Result = vkCreateBuffer(Device, &CrtInfo, nullptr, &VtxBuff);
    MVK_VERIFY(Result == VK_SUCCESS);
  }

  vkGetBufferMemoryRequirements(Device, Buffs[BuffIdx], &MemReq);
  AlignedSize = Detail::alignedSize(MemReq.size, MemReq.alignment);

  auto const PhysicalDevice = Ctx.getPhysicalDevice();
  auto const MemTypeIdx =
      Detail::queryMemType(PhysicalDevice, MemReq.memoryTypeBits,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  MVK_VERIFY(MemTypeIdx.has_value());

  auto MemAllocInfo = VkMemoryAllocateInfo();
  MemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  MemAllocInfo.allocationSize = BuffCount * AlignedSize;
  MemAllocInfo.memoryTypeIndex = MemTypeIdx.value();

  auto Result = vkAllocateMemory(Device, &MemAllocInfo, nullptr, &Mem);
  MVK_VERIFY(Result == VK_SUCCESS);

  for (size_t i = 0; i < BuffCount; ++i)
    vkBindBufferMemory(Device, Buffs[i], Mem, i * AlignedSize);

  State = AllocState::Allocated;
}

void VtxBuff::deallocate() noexcept {
  MVK_VERIFY(State == AllocState::Allocated);

  auto const Device = Ctx.getDevice();

  vkFreeMemory(Device, Mem, nullptr);

  for (auto const Buff : Buffs)
    vkDestroyBuffer(Device, Buff, nullptr);

  State = AllocState::Deallocated;
}

void VtxBuff::moveToGarbage() noexcept {
  State = AllocState::Deallocated;

  Ctx.addBuffersToGarbage(Buffs);
  Ctx.addMemoryToGarbage(Mem);
}

[[nodiscard]] VtxBuff::StageResult
VtxBuff::stage(StagingBuff::MapResult From) noexcept {
  auto const SrcSize = From.Size;

  if (auto ReqSize = Offs[BuffIdx] + SrcSize; ReqSize > LastReqSize) {
    moveToGarbage();
    allocate(2 * ReqSize);
    Offs[BuffIdx] = 0;
  }

  auto const VtxOff = std::exchange(Offs[BuffIdx], Offs[BuffIdx] + SrcSize);

  auto const VtxBuff = Buffs[BuffIdx];

  auto CopyRegion = VkBufferCopy();
  CopyRegion.srcOffset = From.Off;
  CopyRegion.dstOffset = VtxOff;
  CopyRegion.size = SrcSize;

  auto const CmdBuff = Ctx.getCurrentCmdBuff();
  vkCmdCopyBuffer(CmdBuff, From.Buff, VtxBuff, 1, &CopyRegion);

  return {VtxBuff, VtxOff};
}

void VtxBuff::nextBuffer() noexcept {
  BuffIdx = (BuffIdx + 1) % BuffCount;
  Offs[BuffIdx] = 0;
}

} // namespace Mvk::Engine