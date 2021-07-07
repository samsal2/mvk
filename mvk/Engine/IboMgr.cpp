#include "Detail/Misc.hpp"
#include "Engine/IboMgr.hpp"
#include "Utility/Verify.hpp"

namespace Mvk::Engine {

IboMgr::IboMgr(Context &Ctx, VkDeviceSize Size) noexcept
    : State(AllocState::Deallocated), Ctx(Ctx), MemReq(), LastReqSize(0),
      AlignedSize(0), Buffs(), Offs(), Mem(VK_NULL_HANDLE), BuffIdx(0) {
  allocate(Size);
}

IboMgr::~IboMgr() noexcept {
  if (State == AllocState::Allocated)
    deallocate();
}

void IboMgr::allocate(VkDeviceSize Size) noexcept {
  MVK_VERIFY(State == AllocState::Deallocated);

  LastReqSize = Size;

  auto CrtInfo = VkBufferCreateInfo();
  CrtInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  CrtInfo.size = Size;
  CrtInfo.usage =
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  CrtInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  auto const Device = Ctx.getDevice();

  for (auto &IboMgr : Buffs) {
    auto Result = vkCreateBuffer(Device, &CrtInfo, nullptr, &IboMgr);
    MVK_VERIFY(Result == VK_SUCCESS);
  }

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

void IboMgr::deallocate() noexcept {
  MVK_VERIFY(State == AllocState::Allocated);

  auto const Device = Ctx.getDevice();

  vkFreeMemory(Device, Mem, nullptr);
  for (auto const Buff : Buffs) {
    vkDestroyBuffer(Device, Buff, nullptr);
  }

  State = AllocState::Allocated;
}

void IboMgr::moveToGarbage() noexcept {
  State = AllocState::Deallocated;

  Ctx.addBuffersToGarbage(Buffs);
  Ctx.addMemoryToGarbage(Mem);
}

[[nodiscard]] IboMgr::StageResult
IboMgr::stage(StagingMgr::MapResult From) noexcept {
  auto const SrcSize = From.Size;

  if (auto ReqSize = Offs[BuffIdx] + SrcSize; ReqSize > LastReqSize) {
    moveToGarbage();
    allocate(2 * ReqSize);
    Offs[BuffIdx] = 0;
  }

  auto const IdxOff = std::exchange(Offs[BuffIdx], Offs[BuffIdx] + SrcSize);
  auto const IboMgr = Buffs[BuffIdx];

  auto CopyRegion = VkBufferCopy();
  CopyRegion.srcOffset = From.Off;
  CopyRegion.dstOffset = IdxOff;
  CopyRegion.size = SrcSize;

  auto const CmdBuff = Ctx.getCurrentCmdBuff();

  vkCmdCopyBuffer(CmdBuff, From.Buff, IboMgr, 1, &CopyRegion);

  return {IboMgr, IdxOff};
}

void IboMgr::nextBuffer() noexcept {
  BuffIdx = (BuffIdx + 1) % BuffCount;
  Offs[BuffIdx] = 0;
}

} // namespace Mvk::Engine
