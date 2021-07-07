
#ifndef MVK_ENGINE_IDXBUFF_HPP_INCLUDED
#define MVK_ENGINE_IDXBUFF_HPP_INCLUDED

#include "Engine/Context.hpp"
#include "Engine/StagingBuff.hpp"

namespace Mvk::Engine {

class IdxBuff {
public:
  static constexpr size_t BuffCount = 2;

  struct StageResult {
    VkBuffer Buff;
    VkDeviceSize Off;
  };

  IdxBuff(Context &Ctx, VkDeviceSize Size) noexcept;

  IdxBuff(IdxBuff const &Other) noexcept = delete;
  IdxBuff(IdxBuff &&Other) noexcept = delete;

  IdxBuff &operator=(IdxBuff const &Other) noexcept = delete;
  IdxBuff &operator=(IdxBuff &&Other) noexcept = delete;

  ~IdxBuff() noexcept;

  [[nodiscard]] StageResult stage(StagingBuff::MapResult From) noexcept;
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

[[nodiscard]] constexpr Context &IdxBuff::getContext() const noexcept {
  return Ctx;
}

} // namespace Mvk::Engine

#endif