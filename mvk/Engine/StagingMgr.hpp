#pragma once

#include "Engine/Context.hpp"

namespace Mvk::Engine {
class StagingMgr {
public:
  struct MapResult {
    VkBuffer Buff;
    VkDeviceSize Off;
    VkDeviceSize Size;
  };

  static constexpr size_t BuffCount = 2;

  StagingMgr(Context &Ctx, VkDeviceSize Size) noexcept;

  StagingMgr(StagingMgr const &Other) noexcept = delete;
  StagingMgr(StagingMgr &&Other) noexcept = delete;

  StagingMgr &operator=(StagingMgr const &Other) noexcept = delete;
  StagingMgr &operator=(StagingMgr &&Other) noexcept = delete;

  ~StagingMgr() noexcept;

  [[nodiscard]] MapResult map(std::span<std::byte const> Src) noexcept;

  [[nodiscard]] constexpr Context &getContext() const noexcept;
  void nextBuffer() noexcept;

private:
  void allocate(VkDeviceSize Size) noexcept;
  void deallocate() noexcept;
  void moveToGarbage() noexcept;

  enum class AllocState : int { Allocated, Deallocated };

  AllocState State;
  Context &Ctx;
  VkMemoryRequirements MemReq;
  VkDeviceSize LastReqSize;
  VkDeviceSize AlignedSize;
  std::array<VkBuffer, BuffCount> Buffs;
  std::array<VkDeviceSize, BuffCount> Offs;
  VkDeviceMemory Mem;
  std::byte *Data;
  size_t BuffIdx;
};

[[nodiscard]] constexpr Context &StagingMgr::getContext() const noexcept {
  return Ctx;
}

} // namespace Mvk::Engine
