#include "Engine/AllocatorBlock.hpp"

#include "Engine/VulkanContext.hpp"
#include "Utility/Verify.hpp"

#include <optional>

namespace Mvk::Engine
{
  namespace Detail
  {
    static constexpr VkMemoryPropertyFlags getMemProperties( AllocationType Type ) noexcept
    {
      switch ( Type )
      {
        case AllocationType::CpuOnly: return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        case AllocationType::GpuOnly: return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        case AllocationType::CpuToGpu: return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
      }
    }

    [[nodiscard]] static std::optional<uint32_t>
      queryMemType( VkPhysicalDevice PhysicalDevice, uint32_t Filter, VkMemoryPropertyFlags PropFlags )
    {
      auto MemProp = VkPhysicalDeviceMemoryProperties();
      vkGetPhysicalDeviceMemoryProperties( PhysicalDevice, &MemProp );

      auto const TypeCount = MemProp.memoryTypeCount;

      for ( auto i = uint32_t( 0 ); i < TypeCount; ++i )
      {
        auto const & CurrentType   = MemProp.memoryTypes[i];
        auto const   CurrentFlags  = CurrentType.propertyFlags;
        auto const   MatchesFlags  = ( CurrentFlags & PropFlags ) != 0U;
        auto const   MatchesFilter = ( Filter & ( 1U << i ) ) != 0U;

        if ( MatchesFlags && MatchesFilter )
        {
          return i;
        }
      }

      return std::nullopt;
    }

  };  // namespace Detail

  AllocatorBlock::AllocatorBlock( VkDeviceSize Size, AllocationType AllocType, MemoryTypeBits MemType ) noexcept
    : Size( Size )
    , AllocType( AllocType )
    , MemType( MemType )
    , Mem( VK_NULL_HANDLE )
    , OwnerCnt( 0 )
    , Off( 0 )
    , Data( nullptr )
    , IsTombstone( false )
  {
    auto const PhysicalDevice = VulkanContext::the().getPhysicalDevice();

    auto const MemTypeIdx = Detail::queryMemType( PhysicalDevice, MemType, Detail::getMemProperties( AllocType ) );
    MVK_VERIFY( MemTypeIdx.has_value() );

    auto MemAllocInfo            = VkMemoryAllocateInfo();
    MemAllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemAllocInfo.allocationSize  = Size;
    MemAllocInfo.memoryTypeIndex = MemTypeIdx.value();

    auto const Device = VulkanContext::the().getDevice();

    auto Result = vkAllocateMemory( Device, &MemAllocInfo, nullptr, &Mem );
    MVK_VERIFY( Result == VK_SUCCESS );

    if ( AllocType != AllocationType::GpuOnly )
    {
      void * VoidData = nullptr;
      vkMapMemory( Device, Mem, 0, Size, 0, &VoidData );
      Data = reinterpret_cast<std::byte *>( VoidData );
    }
  }

  AllocatorBlock::~AllocatorBlock() noexcept
  {
    auto const Device = VulkanContext::the().getDevice();
    // Memory is implicitly unmaped
    vkFreeMemory( Device, Mem, nullptr );
  }

};  // namespace Mvk::Engine