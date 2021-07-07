#include "Engine/UboBuff.hpp"

#include "Detail/Misc.hpp"
#include "Utility/Types.hpp"
#include "Utility/Verify.hpp"

namespace Mvk::Engine {

UboBuff::UboBuff(Context &Ctx, VkDeviceSize Size) noexcept
    : State(AllocState::Deallocated), Ctx(Ctx), MemReq(), LastReqSize(0),
      AlignedSize(0), DescSets(), Buffs(), Offs(), Mem(VK_NULL_HANDLE),
      BuffIdx(0) {
  allocate(Size);
  write();
}

UboBuff::~UboBuff() noexcept {
  if (State == AllocState::Allocated)
    deallocate();
}

void UboBuff::allocate(VkDeviceSize Size) noexcept {
  MVK_VERIFY(State == AllocState::Deallocated);

  LastReqSize = Size;

  auto BuffCrtInfo = VkBufferCreateInfo();
  BuffCrtInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  BuffCrtInfo.size = Size;
  BuffCrtInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  BuffCrtInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  auto const Device = Ctx.getDevice();

  for (auto &Buff : Buffs) {
    auto Result = vkCreateBuffer(Device, &BuffCrtInfo, nullptr, &Buff);
    MVK_VERIFY(Result == VK_SUCCESS);
  }

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
  Data = Utility::Slice(Utility::forceCastToByte(VoidData),
                        MemAllocInfo.allocationSize);
  MVK_VERIFY(Result == VK_SUCCESS);

  auto const UboDescSetLayout = Ctx.getUboDescSetLayout();

  auto DescriptorSetLayouts =
      std::array<VkDescriptorSetLayout, Context::DynamicBuffCount>();
  std::fill(std::begin(DescriptorSetLayouts), std::end(DescriptorSetLayouts),
            UboDescSetLayout);

  auto const DescriptorPool = Ctx.getDescPool();

  auto DescriptorSetAllocInfo = VkDescriptorSetAllocateInfo();
  DescriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  DescriptorSetAllocInfo.descriptorPool = DescriptorPool;
  DescriptorSetAllocInfo.descriptorSetCount =
      static_cast<uint32_t>(std::size(DescriptorSetLayouts));
  DescriptorSetAllocInfo.pSetLayouts = std::data(DescriptorSetLayouts);

  Result = vkAllocateDescriptorSets(Device, &DescriptorSetAllocInfo,
                                    std::data(DescSets));
  MVK_VERIFY(Result == VK_SUCCESS);

  for (size_t i = 0; i < std::size(Buffs); ++i) {
    vkBindBufferMemory(Device, Buffs[i], Mem, i * AlignedSize);
  }

  State = AllocState::Allocated;
}

void UboBuff::write() noexcept {
  for (size_t i = 0; i < std::size(Buffs); ++i) {
    auto DescriptorBuffInfo = VkDescriptorBufferInfo();
    DescriptorBuffInfo.buffer = Buffs[i];
    DescriptorBuffInfo.offset = 0;
    DescriptorBuffInfo.range = sizeof(PVM);

    auto WriteDescriptorSet = VkWriteDescriptorSet();
    WriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    WriteDescriptorSet.dstSet = DescSets[i];
    WriteDescriptorSet.dstBinding = 0;
    WriteDescriptorSet.dstArrayElement = 0;
    WriteDescriptorSet.descriptorType =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    WriteDescriptorSet.descriptorCount = 1;
    WriteDescriptorSet.pBufferInfo = &DescriptorBuffInfo;
    WriteDescriptorSet.pImageInfo = nullptr;
    WriteDescriptorSet.pTexelBufferView = nullptr;

    auto const Device = Ctx.getDevice();
    vkUpdateDescriptorSets(Device, 1, &WriteDescriptorSet, 0, nullptr);
  }
}

void UboBuff::deallocate() noexcept {
  MVK_VERIFY(State == AllocState::Allocated);

  auto const Device = Ctx.getDevice();
  auto const DescriptorPool = Ctx.getDescPool();

  vkFreeDescriptorSets(Device, DescriptorPool, BuffCount, std::data(DescSets));

  vkFreeMemory(Device, Mem, nullptr);

  for (auto const Buff : Buffs)
    vkDestroyBuffer(Device, Buff, nullptr);

  State = AllocState::Deallocated;
}

void UboBuff::moveToGarbage() noexcept {
  State = AllocState::Deallocated;

  Ctx.addDescriptorSetsToGarbage(DescSets);
  Ctx.addBuffersToGarbage(Buffs);
  Ctx.addMemoryToGarbage(Mem);
}

[[nodiscard]] UboBuff::MapResult
UboBuff::map(Utility::Slice<std::byte const> Src) noexcept {
  auto const SrcSize = std::size(Src);

  Offs[BuffIdx] = Detail::alignedSize(Offs[BuffIdx],
                                      static_cast<uint32_t>(MemReq.alignment));

  if (auto ReqSize = Offs[BuffIdx] + SrcSize; ReqSize > LastReqSize) {
    moveToGarbage();
    allocate(2 * ReqSize);
    write();
    Offs[BuffIdx] = 0;
  }

  auto const BuffOff = std::exchange(Offs[BuffIdx], Offs[BuffIdx] + SrcSize);
  auto const MemOff = BuffIdx * AlignedSize + BuffOff;
  auto const To = Data.subSlice(MemOff, SrcSize);
  std::copy(std::begin(Src), std::end(Src), std::begin(To));

  return {DescSets[BuffIdx], BuffOff};
}

void UboBuff::nextBuffer() noexcept {
  BuffIdx = (BuffIdx + 1) % BuffCount;
  Offs[BuffIdx] = 0;
}

} // namespace Mvk::Engine