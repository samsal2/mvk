
#pragma once

#include "Engine/Context.hpp"
#include "Engine/StagingMgr.hpp"

namespace Mvk::Engine {

class IboMgr {
public:
  static constexpr size_t BuffCount = 2;

  struct StageResult {
    VkBuffer Buff;
    VkDeviceSize Off;
  };

  IboMgr(Context &Ctx, VkDeviceSize Size) noexcept;

  IboMgr(IboMgr const &Other) noexcept = delete;
  IboMgr(IboMgr &&Other) noexcept = delete;

  IboMgr &operator=(IboMgr const &Other) noexcept = delete;
  IboMgr &operator=(IboMgr &&Other) noexcept = delete;

  ~IboMgr() noexcept;

  [[nodiscard]] StageResult stage(StagingMgr::MapResult From) noexcept;
  [[nodiscard]] constexpr Context &getContext() const noexcept;
  void nextBuffer() noexcept;

private:
  void allocate(VkDeviceSize size) noexcept;
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
  size_t BuffIdx;
};

[[nodiscard]] constexpr Context &IboMgr::getContext() const noexcept {
  return Ctx;
}

} // namespace Mvk::Engine
