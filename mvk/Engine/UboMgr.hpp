
#pragma once

#include "Engine/Context.hpp"

namespace Mvk::Engine {
class UboMgr {
public:
  struct MapResult {
    VkDescriptorSet DescriptorSet;
    uint32_t Off;
  };

  static constexpr size_t BuffCount = 2;

  UboMgr(Context &Ctx, VkDeviceSize Size) noexcept;

  UboMgr(UboMgr const &Other) noexcept = delete;
  UboMgr(UboMgr &&Other) noexcept = delete;

  UboMgr &operator=(UboMgr const &Other) noexcept = delete;
  UboMgr &operator=(UboMgr &&Other) noexcept = delete;

  ~UboMgr() noexcept;

  [[nodiscard]] MapResult map(std::span<std::byte const> src) noexcept;

  [[nodiscard]] constexpr Context &getContext() const noexcept;
  void nextBuffer() noexcept;

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
  VkDeviceSize LastReqSize;
  VkDeviceSize AlignedSize;
  std::array<VkDescriptorSet, BuffCount> DescSets;
  std::array<VkBuffer, BuffCount> Buffs;
  std::array<uint32_t, BuffCount> Offs;
  VkDeviceMemory Mem;
  std::byte *Data;
  size_t BuffIdx;
};

[[nodiscard]] constexpr Context &UboMgr::getContext() const noexcept {
  return Ctx;
}

} // namespace Mvk::Engine
