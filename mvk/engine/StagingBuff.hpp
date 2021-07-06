#ifndef MVK_ENGINE_STAGINGBUFF_HPP_INCLUDED
#define MVK_ENGINE_STAGINGBUFF_HPP_INCLUDED

#include "Engine/Context.hpp"

namespace Mvk::Engine {
class StagingBuff {
public:
  struct MapResult {
    VkBuffer Buff;
    VkDeviceSize Off;
    VkDeviceSize Size;
  };

  static constexpr size_t BuffCount = 2;

  StagingBuff(Context &Ctx, VkDeviceSize Size) noexcept;

  StagingBuff(StagingBuff const &Other) noexcept = delete;
  StagingBuff(StagingBuff &&Other) noexcept = delete;

  StagingBuff &operator=(StagingBuff const &Other) noexcept = delete;
  StagingBuff &operator=(StagingBuff &&Other) noexcept = delete;

  ~StagingBuff() noexcept;

  [[nodiscard]] MapResult map(Utility::Slice<std::byte const> Src) noexcept;

  [[nodiscard]] Context &getContext() const noexcept { return Ctx; }

private:
  void allocate(VkDeviceSize Size) noexcept;
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
  Utility::Slice<std::byte> Data;
};

} // namespace Mvk::Engine
#endif