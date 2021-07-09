#pragma once

#include "Utility/Macros.hpp"

#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan.h>

namespace Mvk::Engine
{
  enum class AllocationType
  {
    GpuOnly,
    CpuOnly,
    CpuToGpu
  };

  using MemoryTypeBits = uint32_t;

  class AllocatorBlock
  {
  public:
    static constexpr VkDeviceSize MinSize = 1024 * 1024 * 16;

    AllocatorBlock( VkDeviceSize Size, AllocationType AllocType, MemoryTypeBits MemType ) noexcept;
    MVK_DEFINE_NON_COPYABLE( AllocatorBlock );
    MVK_DEFINE_NON_MOVABLE( AllocatorBlock );
    ~AllocatorBlock() noexcept;

    [[nodiscard]] constexpr VkDeviceSize   getSize() const noexcept;
    [[nodiscard]] constexpr AllocationType getAllocType() const noexcept;
    [[nodiscard]] constexpr MemoryTypeBits getMemType() const noexcept;
    [[nodiscard]] constexpr VkDeviceMemory getMem() const noexcept;
    [[nodiscard]] constexpr size_t         getOwnerCnt() const noexcept;
    [[nodiscard]] constexpr VkDeviceSize   getOff() const noexcept;
    [[nodiscard]] constexpr std::byte *    getData() const noexcept;
    [[nodiscard]] constexpr bool           getIsTombstone() const noexcept;

    constexpr void setOwnerCnt( size_t NewCnt ) noexcept;
    constexpr void setOff( VkDeviceSize Off ) noexcept;
    constexpr void setIsTombstone( bool State ) noexcept;

  private:
    VkDeviceSize   Size;
    AllocationType AllocType;
    MemoryTypeBits MemType;
    VkDeviceMemory Mem;
    size_t         OwnerCnt;
    VkDeviceSize   Off;
    std::byte *    Data;
    bool           IsTombstone;
  };

  [[nodiscard]] constexpr VkDeviceSize AllocatorBlock::getSize() const noexcept
  {
    return Size;
  }

  [[nodiscard]] constexpr AllocationType AllocatorBlock::getAllocType() const noexcept
  {
    return AllocType;
  }

  [[nodiscard]] constexpr MemoryTypeBits AllocatorBlock::getMemType() const noexcept
  {
    return MemType;
  }

  [[nodiscard]] constexpr VkDeviceMemory AllocatorBlock::getMem() const noexcept
  {
    return Mem;
  }

  [[nodiscard]] constexpr size_t AllocatorBlock::getOwnerCnt() const noexcept
  {
    return OwnerCnt;
  }

  [[nodiscard]] constexpr VkDeviceSize AllocatorBlock::getOff() const noexcept
  {
    return Off;
  }

  [[nodiscard]] constexpr std::byte * AllocatorBlock::getData() const noexcept
  {
    return Data;
  }

  [[nodiscard]] constexpr bool AllocatorBlock::getIsTombstone() const noexcept
  {
    return IsTombstone;
  }

  constexpr void AllocatorBlock::setOwnerCnt( size_t NewCnt ) noexcept
  {
    OwnerCnt = NewCnt;
  }

  constexpr void AllocatorBlock::setOff( VkDeviceSize NewOff ) noexcept
  {
    Off = NewOff;
  }

  constexpr void AllocatorBlock::setIsTombstone( bool State ) noexcept
  {
    IsTombstone = State;
  }

}  // namespace Mvk::Engine
