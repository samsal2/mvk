
#pragma once

#include "Engine/StagingBuffObj.hpp"
#include "Utility/Macros.hpp"

namespace Mvk::Engine
{
  class IdxBuffObj
  {
  public:
    struct MapResult
    {
      VkBuffer Buff;
    };

    explicit IdxBuffObj( VkDeviceSize ByteSize, Allocator Alloc = Allocator() ) noexcept;
    MVK_DEFINE_NON_COPYABLE( IdxBuffObj );
    MVK_DEFINE_NON_MOVABLE( IdxBuffObj );
    ~IdxBuffObj() noexcept;

    void map( VkCommandBuffer CmdBuff, std::span<uint32_t const> Data ) noexcept;

    [[nodiscard]] constexpr VkBuffer getBuff() const noexcept
    {
      return Buff;
    }
    [[nodiscard]] constexpr uint32_t getCnt() const noexcept
    {
      return Cnt;
    }

  private:
    std::unique_ptr<StagingBuffObj> Stage;
    VkBuffer                        Buff;
    uint32_t                        Cnt;
    AllocationID                    ID;
  };

}  // namespace Mvk::Engine