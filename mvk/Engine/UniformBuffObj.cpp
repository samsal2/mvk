#include "Engine/UniformBuffObj.hpp"

#include "vulkan/vulkan_core.h"

namespace Mvk::Engine
{
  UniformBuffObj::UniformBuffObj( size_t ByteSize, Allocator Alloc ) noexcept : Alloc( Alloc ), Buff( VK_NULL_HANDLE )
  {
    auto CrtInfo        = VkBufferCreateInfo();
    CrtInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    CrtInfo.size        = ByteSize;
    CrtInfo.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    CrtInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto const Device = VulkanContext::the().getDevice();

    auto Result = vkCreateBuffer( Device, &CrtInfo, nullptr, &Buff );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto Req = VkMemoryRequirements();
    vkGetBufferMemoryRequirements( Device, Buff, &Req );

    auto Allocation = Alloc.allocate( AllocationType::CpuOnly, Req.size, Req.alignment, Req.memoryTypeBits );

    vkBindBufferMemory( Device, Buff, Allocation.Mem, Allocation.Off );

    ID   = Allocation.ID;
    Data = std::span( Allocation.Data, ByteSize );
  }

  UniformBuffObj::~UniformBuffObj() noexcept
  {
    auto const Device = VulkanContext::the().getDevice();
    vkDestroyBuffer( Device, Buff, nullptr );

    Alloc.free( ID );
  }

}  // namespace Mvk::Engine
