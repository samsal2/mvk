#pragma once

#include "Engine/Allocator.hpp"
#include "ShaderTypes.hpp"
#include "Utility/Macros.hpp"
#include "Utility/Verify.hpp"
#include "vulkan/vulkan_core.h"

#include <span>

namespace Mvk::Engine
{
  class StagingBuffObj
  {
  public:
    explicit StagingBuffObj( size_t ByteSize, Allocator Alloc = Allocator() ) noexcept;
    MVK_DEFINE_NON_COPYABLE( StagingBuffObj );
    MVK_DEFINE_NON_MOVABLE( StagingBuffObj );
    ~StagingBuffObj() noexcept;

    constexpr StagingBuffObj & map( std::span<std::byte const> NewData ) noexcept
    {
      MVK_VERIFY( std::size( Data ) >= std::size( NewData ) );
      std::copy( std::begin( NewData ), std::end( NewData ), std::begin( Data ) );
      return *this;
    }

    void copyTo( VkCommandBuffer CmdBuff, VkBuffer ToBuff ) noexcept;
    void copyTo( VkCommandBuffer CmdBuff, VkImage ToImg, size_t Width, size_t Height ) noexcept;

    [[nodiscard]] constexpr Allocator getAllocator() noexcept
    {
      return Alloc;
    }

  private:
    Allocator            Alloc;
    VkBuffer             Buff;
    std::span<std::byte> Data;
    AllocationID         ID;
  };

}  // namespace Mvk::Engine
