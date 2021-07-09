#pragma once

#include "Engine/AllocatorContext.hpp"

namespace Mvk::Engine
{
  class Allocator
  {
  public:
    constexpr explicit Allocator( AllocatorContext & Ctx = AllocatorContext::the() ) noexcept : Ctx( Ctx ) {}

    [[nodiscard]] Allocation allocate( AllocationType Type, VkDeviceSize Size, VkDeviceSize Alignment, MemoryTypeBits BuffMemType ) noexcept
    {
      return Ctx.allocate( Type, Size, Alignment, BuffMemType );
    }

    void free( AllocationID ID ) noexcept
    {
      Ctx.free( ID );
    }

  private:
    AllocatorContext & Ctx;
  };

}  // namespace Mvk::Engine