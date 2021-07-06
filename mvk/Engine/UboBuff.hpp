
#ifndef MVK_ENGINE_UBOBUFF_HPP_INCLUDED
#define MVK_ENGINE_UBOBUFF_HPP_INCLUDED

#include "Engine/Context.hpp"

namespace Mvk::Engine {
class UboBuff {
public:
  struct MapResult {
    VkDescriptorSet DescriptorSet;
    uint32_t Off;
  };

  static constexpr size_t BuffCount = 2;

  UboBuff(Context &Ctx, VkDeviceSize Size) noexcept;

  UboBuff(UboBuff const &Other) noexcept = delete;
  UboBuff(UboBuff &&Other) noexcept = delete;

  UboBuff &operator=(UboBuff const &Other) noexcept = delete;
  UboBuff &operator=(UboBuff &&Other) noexcept = delete;

  ~UboBuff() noexcept;

  [[nodiscard]] MapResult map(Utility::Slice<std::byte const> src) noexcept;

  [[nodiscard]] Context &getContext() const noexcept { return Ctx; }

private:
  void allocate(VkDeviceSize size) noexcept;

  // Updates the descriptor sets
  void write() noexcept;

  void deallocate() noexcept;
  void moveToGarbage() noexcept;

  enum class AllocState : int { Allocated, Deallocated };

  AllocState State;
  Context &Ctx;
  VkMemoryRequirements MemReq;
  VkDeviceSize AlignedSize;
  std::array<VkDescriptorSet, BuffCount> DescSets;
  std::array<VkBuffer, BuffCount> Buffs;
  std::array<uint32_t, BuffCount> Offs;
  VkDeviceMemory Mem;
  Utility::Slice<std::byte> Data;
};

} // namespace Mvk::Engine
#endif