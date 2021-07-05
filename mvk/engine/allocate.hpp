#ifndef MVK_ENGINE_ALLOCATE_HPP_INCLUDED
#define MVK_ENGINE_ALLOCATE_HPP_INCLUDED

#include "engine/context.hpp"

namespace mvk::engine
{
  struct StagingAlloc
  {
    VkBuffer     Buff;
    VkDeviceSize Off;
    VkDeviceSize Size;
  };

  struct VtxAlloc
  {
    VkBuffer     Buff;
    VkDeviceSize Off;
  };

  struct IdxAlloc
  {
    VkBuffer     Buff;
    VkDeviceSize Off;
  };

  struct UboAlloc
  {
    VkDescriptorSet DescriptorSet;
    uint32_t        Off;
  };

  void crtVtxBuffAndMem(InOut<Context> Ctx, VkDeviceSize Size) noexcept;
  void dtyVtxBuffAndMem(InOut<Context> Ctx) noexcept;
  void crtIdxBuffAndMem(InOut<Context> Ctx, VkDeviceSize Size) noexcept;
  void dtyIdxBuffAndMem(InOut<Context> Ctx) noexcept;
  void crtStagingBuffAndMem(InOut<Context> Ctx, VkDeviceSize Size) noexcept;
  void dtyStagingBuffAndMem(InOut<Context> Ctx) noexcept;
  void crtStagingBuffMemAndSets(InOut<Context> Ctx, VkDeviceSize Size) noexcept;
  void dtyBuffMemAndSet(InOut<Context> Ctx) noexcept;

  void dtyGarbageBuff(InOut<Context> Ctx) noexcept;
  void dtyGarbageMem(InOut<Context> Ctx) noexcept;
  void dtyGarbageSets(InOut<Context> Ctx) noexcept;

  void mvToGarbageBuff(InOut<Context> Ctx, utility::Slice<VkBuffer> Buffers) noexcept;
  void mvToGarbageSets(InOut<Context> Ctx, utility::Slice<VkDescriptorSet> bets) noexcept;
  void mvToGarbageMem(InOut<Context> Ctx, VkDeviceMemory Mem) noexcept;

  [[nodiscard]] StagingAlloc allocStaging(InOut<Context> Ctx, utility::Slice<std::byte const> Src) noexcept;
  [[nodiscard]] VtxAlloc     allocVtx(InOut<Context> Ctx, StagingAlloc Alloc) noexcept;
  [[nodiscard]] IdxAlloc     allocIdx(InOut<Context> Ctx, StagingAlloc Alloc) noexcept;
  [[nodiscard]] UboAlloc     allocUbo(InOut<Context> Ctx, utility::Slice<std::byte const> Src) noexcept;

  template <size_t Size>
  [[nodiscard]] std::array<VkDescriptorSet, Size> allocDescriptorSets(In<Context>           Ctx,
                                                                      VkDescriptorSetLayout Lay) noexcept;

  template <size_t Size>
  [[nodiscard]] std::array<VkCommandBuffer, Size> allocCmdBuff(In<Context> Ctx, VkCommandBufferLevel Lvl) noexcept;

  [[nodiscard]] VkCommandBuffer allocSingleUseCmdBuf(In<Context> Ctx) noexcept;

  void nextBuffer(InOut<Context> Ctx) noexcept;

}  // namespace mvk::engine

namespace mvk::engine
{
  template <size_t Size>
  [[nodiscard]] std::array<VkDescriptorSet, Size> allocDescriptorSets(In<Context>           Ctx,
                                                                      VkDescriptorSetLayout Lay) noexcept
  {
    auto DescriptorSetLays = std::array<VkDescriptorSetLayout, Size>();
    std::fill(std::begin(DescriptorSetLays), std::end(DescriptorSetLays), Lay);

    auto const AllocInfo = [&Ctx, &DescriptorSetLays]
    {
      auto info               = VkDescriptorSetAllocateInfo();
      info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      info.descriptorPool     = Ctx->DescriptorPool;
      info.descriptorSetCount = static_cast<uint32_t>(std::size(DescriptorSetLays));
      info.pSetLayouts        = std::data(DescriptorSetLays);
      return info;
    }();

    auto DescriptorSets = std::array<VkDescriptorSet, Size>();

    [[maybe_unused]] auto Result = vkAllocateDescriptorSets(Ctx->Dev, &AllocInfo, std::data(DescriptorSets));
    MVK_VERIFY(Result == VK_SUCCESS);
    return DescriptorSets;
  }

  template <size_t Size>
  [[nodiscard]] std::array<VkCommandBuffer, Size> allocCmdBuff(In<Context> Ctx, VkCommandBufferLevel Lvl) noexcept
  {
    auto const AllocInfo = [&Ctx, Lvl]
    {
      auto info               = VkCommandBufferAllocateInfo();
      info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      info.commandPool        = Ctx->CmdPool;
      info.level              = Lvl;
      info.commandBufferCount = Size;
      return info;
    }();

    auto CommandBuffers = std::array<VkCommandBuffer, Size>();

    [[maybe_unused]] auto result = vkAllocateCommandBuffers(Ctx->Dev, &AllocInfo, std::data(CommandBuffers));
    MVK_VERIFY(result == VK_SUCCESS);
    return CommandBuffers;
  }

}  // namespace mvk::engine

#endif
