#pragma once

#include "Engine/Context.hpp"
#include "Engine/StagingMgr.hpp"
#include "Utility/Verify.hpp"

namespace Mvk::Engine {
// TODO(samuel): RAII
class VboMgr {
public:
  static constexpr size_t BuffCount = 2;

  struct StageResult {
    VkBuffer Buff;
    VkDeviceSize Off;
  };

  VboMgr(Context &Ctx, VkDeviceSize Size) noexcept;

  VboMgr(VboMgr const &Other) noexcept = delete;
  VboMgr(VboMgr &&Other) noexcept = delete;

  VboMgr &operator=(VboMgr const &Other) noexcept = delete;
  VboMgr &operator=(VboMgr &&Other) noexcept = delete;

  ~VboMgr() noexcept;

  [[nodiscard]] StageResult stage(StagingMgr::MapResult From) noexcept;

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
  size_t BuffIdx;
};

[[nodiscard]] constexpr Context &VboMgr::getContext() const noexcept {
  return Ctx;
}

} // namespace Mvk::Engine
