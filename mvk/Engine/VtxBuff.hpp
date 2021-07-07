#ifndef MVK_ENGINE_VTXBUFF_HPP_INCLUDED
#define MVK_ENGINE_VTXBUFF_HPP_INCLUDED

#include "Engine/Context.hpp"
#include "Engine/StagingBuff.hpp"
#include "Utility/Verify.hpp"

namespace Mvk::Engine {
// TODO(samuel): RAII
class VtxBuff {
public:
  static constexpr size_t BuffCount = 2;

  struct StageResult {
    VkBuffer Buff;
    VkDeviceSize Off;
  };

  VtxBuff(Context &Ctx, VkDeviceSize Size) noexcept;

  VtxBuff(VtxBuff const &Other) noexcept = delete;
  VtxBuff(VtxBuff &&Other) noexcept = delete;

  VtxBuff &operator=(VtxBuff const &Other) noexcept = delete;
  VtxBuff &operator=(VtxBuff &&Other) noexcept = delete;

  ~VtxBuff() noexcept;

  [[nodiscard]] StageResult stage(StagingBuff::MapResult From) noexcept;

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

[[nodiscard]] constexpr Context &VtxBuff::getContext() const noexcept {
  return Ctx;
}

} // namespace Mvk::Engine

#endif
