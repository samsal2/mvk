
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
  [[nodiscard]] Context &getContext() const noexcept { return Ctx; }

private:
  void allocate(VkDeviceSize size) noexcept;
  void deallocate() noexcept;
  void moveToGarbage() noexcept;

  enum class AllocState : int { Allocated, Deallocated };

  AllocState State;

  Context &Ctx;
  VkMemoryRequirements MemReq;
  VkDeviceSize AlignedSize;
  std::array<VkBuffer, BuffCount> Buffs;
  std::array<VkDeviceSize, BuffCount> Offs;
  VkDeviceMemory Mem;
};

} // namespace Mvk::Engine

#endif