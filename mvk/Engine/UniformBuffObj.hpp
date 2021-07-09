#pragma once

#include "Engine/Allocator.hpp"
#include "ShaderTypes.hpp"
#include "Utility/Macros.hpp"
#include "Utility/Verify.hpp"

#include <span>

namespace Mvk::Engine
{
  class UniformBuffObj
  {
  public:
    struct MapResult
    {
      VkBuffer Buff;
    };

    explicit UniformBuffObj( size_t ByteSize, Allocator Alloc = Allocator() ) noexcept;
    MVK_DEFINE_NON_COPYABLE( UniformBuffObj );
    MVK_DEFINE_NON_MOVABLE( UniformBuffObj );
    ~UniformBuffObj() noexcept;

    void map( std::span<std::byte const> NewData ) noexcept
    {
      MVK_VERIFY( std::size( Data ) >= std::size( NewData ) );
      std::copy( std::begin( NewData ), std::end( NewData ), std::begin( Data ) );
    }

    [[nodiscard]] constexpr VkBuffer getBuffer() const noexcept
    {
      return Buff;
    }

  private:
    Allocator            Alloc;
    VkBuffer             Buff;
    std::span<std::byte> Data;
    AllocationID         ID;
  };

}  // namespace Mvk::Engine
