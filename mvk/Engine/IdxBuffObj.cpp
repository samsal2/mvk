
#include "Engine/IdxBuffObj.hpp"

#include "Engine/VulkanContext.hpp"

namespace Mvk::Engine
{
  IdxBuffObj::IdxBuffObj( VkDeviceSize ByteSize, Allocator Alloc ) noexcept
    : Stage( std::make_unique<StagingBuffObj>( ByteSize, Alloc ) ), Buff( VK_NULL_HANDLE )
  {
    auto CrtInfo        = VkBufferCreateInfo();
    CrtInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    CrtInfo.size        = ByteSize;
    CrtInfo.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    CrtInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto const Device = VulkanContext::the().getDevice();

    auto Result = vkCreateBuffer( Device, &CrtInfo, nullptr, &Buff );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto Req = VkMemoryRequirements();
    vkGetBufferMemoryRequirements( Device, Buff, &Req );

    auto Allocation = Stage->getAllocator().allocate( AllocationType::GpuOnly, Req.size, Req.alignment, Req.memoryTypeBits );

    vkBindBufferMemory( Device, Buff, Allocation.Mem, Allocation.Off );

    ID = Allocation.ID;
  }

  void IdxBuffObj::map( VkCommandBuffer CmdBuff, std::span<uint32_t const> Data ) noexcept
  {
    Cnt = std::size( Data );
    Stage->map( std::as_bytes( Data ) ).copyTo( CmdBuff, Buff );
  }

  IdxBuffObj::~IdxBuffObj() noexcept
  {
    auto const Device = VulkanContext::the().getDevice();
    vkDestroyBuffer( Device, Buff, nullptr );

    Stage->getAllocator().free( ID );
  }

}  // namespace Mvk::Engine
