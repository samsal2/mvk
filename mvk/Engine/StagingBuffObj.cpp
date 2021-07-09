#include "Engine/StagingBuffObj.hpp"

#include "Engine/VulkanContext.hpp"

#include <iostream>

namespace Mvk::Engine
{
  StagingBuffObj::StagingBuffObj( size_t ByteSize, Allocator Alloc ) noexcept : Alloc( Alloc ), Buff( VK_NULL_HANDLE )
  {
    auto CrtInfo        = VkBufferCreateInfo();
    CrtInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    CrtInfo.size        = ByteSize;
    CrtInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    CrtInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto const Device = VulkanContext::the().getDevice();

    auto Result = vkCreateBuffer( Device, &CrtInfo, nullptr, &Buff );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto Req = VkMemoryRequirements();
    vkGetBufferMemoryRequirements( Device, Buff, &Req );

    auto Allocation = Alloc.allocate( AllocationType::CpuToGpu, Req.size, Req.alignment, Req.memoryTypeBits );

    vkBindBufferMemory( Device, Buff, Allocation.Mem, Allocation.Off );

    ID   = Allocation.ID;
    Data = std::span( Allocation.Data, ByteSize );
  }

  StagingBuffObj::~StagingBuffObj() noexcept
  {
    auto const Device = VulkanContext::the().getDevice();
    vkDestroyBuffer( Device, Buff, nullptr );

    Alloc.free( ID );
  }

  void StagingBuffObj::copyTo( VkCommandBuffer CmdBuff, VkBuffer ToBuff ) noexcept
  {
    auto CopyRegion      = VkBufferCopy();
    CopyRegion.srcOffset = 0;
    CopyRegion.dstOffset = 0;
    CopyRegion.size      = std::size( Data );

    vkCmdCopyBuffer( CmdBuff, Buff, ToBuff, 1, &CopyRegion );
  }

  void StagingBuffObj::copyTo( VkCommandBuffer CmdBuff, VkImage ToImg, size_t Width, size_t Height ) noexcept
  {
    auto CopyRegion                            = VkBufferImageCopy();
    CopyRegion.bufferOffset                    = 0;
    CopyRegion.bufferRowLength                 = 0;
    CopyRegion.bufferImageHeight               = 0;
    CopyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    CopyRegion.imageSubresource.mipLevel       = 0;
    CopyRegion.imageSubresource.baseArrayLayer = 0;
    CopyRegion.imageSubresource.layerCount     = 1;
    CopyRegion.imageOffset.x                   = 0;
    CopyRegion.imageOffset.y                   = 0;
    CopyRegion.imageOffset.z                   = 0;
    CopyRegion.imageExtent.width               = Width;
    CopyRegion.imageExtent.height              = Height;
    CopyRegion.imageExtent.depth               = 1;

    vkCmdCopyBufferToImage( CmdBuff, Buff, ToImg, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &CopyRegion );
  }

};  // namespace Mvk::Engine
