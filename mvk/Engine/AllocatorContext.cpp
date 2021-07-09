#include "Engine/AllocatorContext.hpp"

#include "Utility/Verify.hpp"

namespace Mvk::Engine
{
  namespace Detail
  {
    [[nodiscard]] constexpr size_t alignedSize( size_t Size, size_t Alignment ) noexcept
    {
      if ( auto Mod = Size % Alignment; Mod != 0 )
      {
        return Size + Alignment - Mod;
      }

      return Size;
    }

  }  // namespace Detail

  void AllocatorContext::initialize( Utility::Badge<VulkanContext> ) noexcept {}

  [[nodiscard]] Allocation
    AllocatorContext::allocate( AllocationType Type, VkDeviceSize Size, VkDeviceSize Alignment, MemoryTypeBits BuffMemType ) noexcept
  {
    auto const AlignedSize = Detail::alignedSize( Size, Alignment );

    for ( auto ID = AllocationID( 0 ); ID != std::size( Blocks ); ++ID )
    {
      auto & Block = Blocks[ID];

      // Deleted block slots will be used on the next step if no fit blocks are found
      if ( Block == nullptr )
      {
        continue;
      }

      // If the memory type and allocation type don't match, go next
      if ( Block->getMemType() != BuffMemType || Block->getAllocType() != Type )
      {
        continue;
      }

      auto const BlockSize = Block->getSize();
      // If the allocation doesn't fit in the block size go next
      if ( BlockSize < AlignedSize )
      {
        continue;
      }

      // If the block is marked as "Tombstone", and the allocation fits use this block
      if ( Block->getIsTombstone() )
      {
        Block->setOff( AlignedSize );
        Block->setIsTombstone( false );
        Block->setOwnerCnt( 1 );
        return { ID, Block->getMem(), 0, Block->getData() };
      }

      // If the allocation doesn't fit in the remaining space go next
      auto const BlockAlignedOff = Detail::alignedSize( Block->getOff(), Alignment );
      if ( BlockSize - BlockAlignedOff < AlignedSize )
      {
        continue;
      }

      Block->setOff( BlockAlignedOff + AlignedSize );
      auto const OwnrCnt = Block->getOwnerCnt();
      Block->setOwnerCnt( OwnrCnt + 1 );
      return { ID, Block->getMem(), BlockAlignedOff, Block->getData() + BlockAlignedOff };
    }

    // No fit blocks where found, find any empty slots to allcate a new block
    for ( auto ID = AllocationID( 0 ); ID != std::size( Blocks ); ++ID )
    {
      if ( auto & Block = Blocks[ID]; Block == nullptr )
      {
        auto const BlockSize = std::max( Size * 2, AllocatorBlock::MinSize );

        Block = std::make_unique<AllocatorBlock>( BlockSize, Type, BuffMemType );

        Block->setOff( AlignedSize );
        Block->setOwnerCnt( 1 );
        return { ID, Block->getMem(), 0, Block->getData() };
      }
    }

    // No fit blocks found, not empty slots
    auto const BlockSize = std::max( Size * 2, AllocatorBlock::MinSize );

    Blocks.push_back( std::make_unique<AllocatorBlock>( BlockSize, Type, BuffMemType ) );
    auto & Block = Blocks.back();
    Block->setOff( AlignedSize );
    Block->setOwnerCnt( 1 );
    return { std::size( Blocks ) - 1, Block->getMem(), 0, Block->getData() };
  }

  void AllocatorContext::free( AllocationID FreeID ) noexcept
  {
    // Do the cleanup
    for ( auto ID = AllocationID( 0 ); ID != std::size( Blocks ); ++ID )
    {
      // If the block was marked tombstone the last free, and it wasn't used free the block
      if ( auto & Block = Blocks[ID]; Block != nullptr )
      {
        if ( Block->getIsTombstone() )
        {
          Block.reset();
        }
      }
    }

    auto & Block = Blocks[FreeID];

    MVK_VERIFY( Block );

    auto const OwnerCnt = Block->getOwnerCnt();

    MVK_VERIFY( OwnerCnt != 0 );

    if ( OwnerCnt == 1 )
    {
      Block->setIsTombstone( true );
    }

    Block->setOwnerCnt( OwnerCnt - 1 );
  }

  void AllocatorContext::shutdown() noexcept
  {
    Blocks.clear();
  }

}  // namespace Mvk::Engine