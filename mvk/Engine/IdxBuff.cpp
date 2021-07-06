#include "Engine/IdxBuff.hpp"

#include "Detail/Misc.hpp"
#include "Utility/Verify.hpp"

namespace Mvk::Engine {

IdxBuff::IdxBuff(Context &Ctx, VkDeviceSize Size) noexcept
    : State(AllocState::Deallocated), Ctx(Ctx), MemReq(), AlignedSize(0),
      Buffs(), Offs(), Mem(VK_NULL_HANDLE) {
  allocate(Size);
}

IdxBuff::~IdxBuff() noexcept {
  if (State == AllocState::Allocated)
    deallocate();
}

void IdxBuff::allocate(VkDeviceSize Size) noexcept {
  MVK_VERIFY(State == AllocState::Deallocated);

  auto CrtInfo = VkBufferCreateInfo();
  CrtInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  CrtInfo.size = Size;
  CrtInfo.usage =
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  CrtInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  auto const Device = Ctx.getDevice();

  for (auto &IdxBuff : Buffs) {
    auto Result = vkCreateBuffer(Device, &CrtInfo, nullptr, &IdxBuff);
    MVK_VERIFY(Result == VK_SUCCESS);
  }

  auto const BuffIdx = Ctx.getCurrentBuffIdx();

  vkGetBufferMemoryRequirements(Device, Buffs[BuffIdx], &MemReq);
  AlignedSize = Detail::alignedSize(MemReq.size, MemReq.alignment);

  auto const PhysicalDevice = Ctx.getPhysicalDevice();

  auto const MemTypeIdx =
      Detail::queryMemType(PhysicalDevice, MemReq.memoryTypeBits,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  MVK_VERIFY(MemTypeIdx.has_value());

  auto IdxMemAllocInfo = VkMemoryAllocateInfo();
  IdxMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  IdxMemAllocInfo.allocationSize = BuffCount * AlignedSize;
  IdxMemAllocInfo.memoryTypeIndex = MemTypeIdx.value();

  auto Result = vkAllocateMemory(Device, &IdxMemAllocInfo, nullptr, &Mem);
  MVK_VERIFY(Result == VK_SUCCESS);

  for (size_t i = 0; i < BuffCount; ++i)
    vkBindBufferMemory(Device, Buffs[i], Mem, i * AlignedSize);

  State = AllocState::Allocated;
}

void IdxBuff::deallocate() noexcept {
  MVK_VERIFY(State == AllocState::Allocated);

  auto const Device = Ctx.getDevice();

  vkFreeMemory(Device, Mem, nullptr);
  for (auto const Buff : Buffs) {
    vkDestroyBuffer(Device, Buff, nullptr);
  }

  State = AllocState::Allocated;
}

void IdxBuff::moveToGarbage() noexcept {
  State = AllocState::Deallocated;

  Ctx.addBuffersToGarbage(Buffs);
  Ctx.addMemoryToGarbage(Mem);
}

[[nodiscard]] IdxBuff::StageResult
IdxBuff::stage(StagingBuff::MapResult From) noexcept {
  auto const BuffIdx = Ctx.getCurrentBuffIdx();
  auto const SrcSize = From.Size;

  if (auto ReqSize = Offs[BuffIdx] + SrcSize; ReqSize > AlignedSize) {
    moveToGarbage();
    allocate(2 * ReqSize);
    Offs[BuffIdx] = 0;
  }

  auto const IdxOff = std::exchange(Offs[BuffIdx], Offs[BuffIdx] + SrcSize);

  auto const IdxBuff = Buffs[BuffIdx];

  auto CopyRegion = VkBufferCopy();
  CopyRegion.srcOffset = From.Off;
  CopyRegion.dstOffset = IdxOff;
  CopyRegion.size = SrcSize;

  auto const CmdBuff = Ctx.getCurrentCmdBuff();

  vkCmdCopyBuffer(CmdBuff, From.Buff, IdxBuff, 1, &CopyRegion);

  return {IdxBuff, IdxOff};
}

} // namespace Mvk::Engine
