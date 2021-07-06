#include "Engine/StagingBuff.hpp"

#include "Detail/Misc.hpp"
#include "Utility/Types.hpp"

namespace Mvk::Engine {

StagingBuff::StagingBuff(Context &Ctx, VkDeviceSize Size) noexcept
    : State(AllocState::Deallocated), Ctx(Ctx), MemReq(), AlignedSize(0),
      Buffs(), Offs(), Mem(VK_NULL_HANDLE), Data() {
  allocate(Size);
}

StagingBuff::~StagingBuff() noexcept {
  if (State == AllocState::Allocated)
    deallocate();
}

void StagingBuff::allocate(VkDeviceSize size) noexcept {
  MVK_VERIFY(State == AllocState::Deallocated);

  auto CrtInfo = VkBufferCreateInfo();
  CrtInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  CrtInfo.size = size;
  CrtInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  CrtInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  auto const Device = Ctx.getDevice();

  for (auto &Buff : Buffs) {
    auto Result = vkCreateBuffer(Device, &CrtInfo, nullptr, &Buff);
    MVK_VERIFY(Result == VK_SUCCESS);
  }

  auto const BuffIdx = Ctx.getCurrentBuffIdx();

  vkGetBufferMemoryRequirements(Device, Buffs[BuffIdx], &MemReq);
  AlignedSize = Detail::alignedSize(MemReq.size, MemReq.alignment);

  auto const PhysicalDevice = Ctx.getPhysicalDevice();

  auto const MemTypeIdx =
      Detail::queryMemType(PhysicalDevice, MemReq.memoryTypeBits,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  MVK_VERIFY(MemTypeIdx.value());

  auto MemAllocInfo = VkMemoryAllocateInfo();
  MemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  MemAllocInfo.allocationSize = BuffCount * AlignedSize;
  MemAllocInfo.memoryTypeIndex = MemTypeIdx.value();

  auto Result = vkAllocateMemory(Device, &MemAllocInfo, nullptr, &Mem);
  MVK_VERIFY(Result == VK_SUCCESS);

  void *VoidData = nullptr;

  Result = vkMapMemory(Device, Mem, 0, VK_WHOLE_SIZE, 0, &VoidData);
  MVK_VERIFY(Result == VK_SUCCESS);

  Data = Utility::Slice(Utility::forceCastToByte(VoidData),
                        MemAllocInfo.allocationSize);

  for (size_t i = 0; i < Context::DynamicBuffCount; ++i)
    vkBindBufferMemory(Device, Buffs[i], Mem, i * AlignedSize);

  State = AllocState::Allocated;
}

void StagingBuff::deallocate() noexcept {
  MVK_VERIFY(State == AllocState::Allocated);

  auto const Device = Ctx.getDevice();

  vkFreeMemory(Device, Mem, nullptr);

  for (auto const Buff : Buffs)
    vkDestroyBuffer(Device, Buff, nullptr);

  State = AllocState::Deallocated;
}

void StagingBuff::moveToGarbage() noexcept {
  State = AllocState::Deallocated;

  Ctx.addBuffersToGarbage(Buffs);
  Ctx.addMemoryToGarbage(Mem);
}

// Tries simple map, if it fails it reallocates * 2 de memory required
[[nodiscard]] StagingBuff::MapResult
StagingBuff::map(Utility::Slice<std::byte const> Src) noexcept {
  auto const BuffIdx = Ctx.getCurrentBuffIdx();
  auto const SrcSize = std::size(Src);

  Offs[BuffIdx] = Detail::alignedSize(Offs[BuffIdx], MemReq.alignment);

  if (auto ReqSize = Offs[BuffIdx] + SrcSize; ReqSize > AlignedSize) {
    // TODO(samuel): moveToGarbage should be part of Ctx
    moveToGarbage();
    allocate(ReqSize * 2);
    Offs[BuffIdx] = 0;
  }

  auto const StagingOff = std::exchange(Offs[BuffIdx], Offs[BuffIdx] + SrcSize);
  auto const MemOff = BuffIdx * AlignedSize + StagingOff;
  auto const To = Data.subSlice(MemOff, SrcSize);
  std::copy(std::begin(Src), std::end(Src), std::begin(To));

  return {Buffs[BuffIdx], StagingOff, SrcSize};
}

} // namespace Mvk::Engine