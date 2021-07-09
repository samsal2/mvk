#pragma once

#include "Engine/AllocatorBlock.hpp"
#include "Engine/VulkanContext.hpp"
#include "Utility/Badge.hpp"
#include "Utility/Macros.hpp"
#include "Utility/Singleton.hpp"

#include <memory>
#include <vector>

namespace Mvk::Engine
{
  using AllocationID = size_t;

  struct Allocation
  {
    AllocationID   ID;
    VkDeviceMemory Mem;
    VkDeviceSize   Off;
    std::byte *    Data;
  };

  // TODO(samuel): make part of Context
  class AllocatorContext : public Utility::Singleton<AllocatorContext>
  {
  public:
    using Utility::Singleton<AllocatorContext>::Singleton;

    void initialize( Utility::Badge<VulkanContext> ) noexcept;

    [[nodiscard]] Allocation
      allocate( AllocationType Type, VkDeviceSize Size, VkDeviceSize Alignment, MemoryTypeBits BuffMemType ) noexcept;

    void free( AllocationID ID ) noexcept;

    void shutdown() noexcept;

  private:
    std::vector<std::unique_ptr<AllocatorBlock>> Blocks;
  };

}  // namespace Mvk::Engine