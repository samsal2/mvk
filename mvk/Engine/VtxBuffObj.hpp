#pragma once

#include "Engine/StagingBuffObj.hpp"
#include "Utility/Macros.hpp"

namespace Mvk::Engine
{
  class VtxBuffObj
  {
  public:
    struct MapResult
    {
      VkBuffer Buff;
    };

    explicit VtxBuffObj( VkDeviceSize ByteSize, Allocator Alloc = Allocator() ) noexcept;
    MVK_DEFINE_NON_COPYABLE( VtxBuffObj );
    MVK_DEFINE_NON_MOVABLE( VtxBuffObj );
    ~VtxBuffObj() noexcept;

    void map( VkCommandBuffer CmdBuff, std::span<std::byte const> Data ) noexcept;

    [[nodiscard]] constexpr VkBuffer getBuff() const noexcept
    {
      return Buff;
    }

  private:
    StagingBuffObj Stage;
    VkBuffer       Buff;
    AllocationID   ID;
  };

}  // namespace Mvk::Engine
