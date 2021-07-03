#include "engine/allocate.hpp"

#include "detail/helpers.hpp"
#include "detail/misc.hpp"

namespace mvk::engine
{
  [[nodiscard]] types::unique_command_buffer allocate_single_use_command_buffer( context const & ctx ) noexcept
  {
    auto info               = VkCommandBufferAllocateInfo();
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool        = types::get( ctx.command_pool_ );
    info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = 1;
    return std::move( types::allocate_unique_command_buffers( types::get( ctx.device_ ), info )[ 0 ] );
  }

  void create_vertex_buffers_and_memories( context & ctx, types::device_size size ) noexcept
  {
    auto const vertex_buffer_create_info = [ size ]
    {
      auto info        = VkBufferCreateInfo();
      info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      info.size        = size;
      info.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
      info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      return info;
    }();

    auto const create_buffer = [ &ctx, &vertex_buffer_create_info ]
    {
      return types::create_unique_buffer( types::get( ctx.device_ ), vertex_buffer_create_info );
    };

    std::generate( std::begin( ctx.vertex_buffers_ ), std::end( ctx.vertex_buffers_ ), create_buffer );

    vkGetBufferMemoryRequirements( types::get( ctx.device_ ),
                                   types::get( ctx.vertex_buffers_[ ctx.current_buffer_index_ ] ),
                                   &ctx.vertex_memory_requirements_ );

    ctx.vertex_aligned_size_ =
      detail::aligned_size( ctx.vertex_memory_requirements_.size, ctx.vertex_memory_requirements_.alignment );

    auto const memory_type_index = detail::find_memory_type( types::get( ctx.physical_device_ ),
                                                             ctx.vertex_memory_requirements_.memoryTypeBits,
                                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    MVK_VERIFY( memory_type_index.has_value() );

    auto vertex_memory_allocate_info = [ &ctx, memory_type_index ]
    {
      auto info            = VkMemoryAllocateInfo();
      info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      info.allocationSize  = context::dynamic_buffer_count * ctx.vertex_aligned_size_;
      info.memoryTypeIndex = memory_type_index.value();
      return info;
    }();

    ctx.vertex_memory_ = types::create_unique_device_memory( types::get( ctx.device_ ), vertex_memory_allocate_info );

    for ( size_t i = 0; i < context::dynamic_buffer_count; ++i )
    {
      auto const & current_buffer = ctx.vertex_buffers_[ i ];
      vkBindBufferMemory( types::parent( current_buffer ),
                          types::get( current_buffer ),
                          types::get( ctx.vertex_memory_ ),
                          i * ctx.vertex_aligned_size_ );
    }
  }

  void create_index_buffers_and_memories( context & ctx, types::device_size size ) noexcept
  {
    auto const index_buffer_create_info = [ size ]
    {
      auto info        = VkBufferCreateInfo();
      info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      info.size        = size;
      info.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
      info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      return info;
    }();

    auto const create_buffer = [ &ctx, &index_buffer_create_info ]
    {
      return types::create_unique_buffer( types::get( ctx.device_ ), index_buffer_create_info );
    };

    std::generate( std::begin( ctx.index_buffers_ ), std::end( ctx.index_buffers_ ), create_buffer );

    vkGetBufferMemoryRequirements( types::get( ctx.device_ ),
                                   types::get( ctx.index_buffers_[ ctx.current_buffer_index_ ] ),
                                   &ctx.index_memory_requirements_ );

    ctx.index_aligned_size_ =
      detail::aligned_size( ctx.index_memory_requirements_.size, ctx.index_memory_requirements_.alignment );

    auto const memory_type_index = detail::find_memory_type( types::get( ctx.physical_device_ ),
                                                             ctx.index_memory_requirements_.memoryTypeBits,
                                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    MVK_VERIFY( memory_type_index.has_value() );

    auto index_memory_allocate_info = [ &ctx, memory_type_index ]
    {
      auto info            = VkMemoryAllocateInfo();
      info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      info.allocationSize  = context::dynamic_buffer_count * ctx.index_aligned_size_;
      info.memoryTypeIndex = memory_type_index.value();
      return info;
    }();

    ctx.index_memory_ = types::create_unique_device_memory( types::get( ctx.device_ ), index_memory_allocate_info );

    for ( size_t i = 0; i < context::dynamic_buffer_count; ++i )
    {
      auto const & current_buffer = ctx.index_buffers_[ i ];
      vkBindBufferMemory( types::parent( current_buffer ),
                          types::get( current_buffer ),
                          types::get( ctx.index_memory_ ),
                          i * ctx.index_aligned_size_ );
    }
  }

  void create_staging_buffers_and_memories( context & ctx, types::device_size size ) noexcept
  {
    auto const staging_buffer_create_info = [ size ]
    {
      auto info        = VkBufferCreateInfo();
      info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      info.size        = size;
      info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
      info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      return info;
    }();

    auto const create_buffer = [ &ctx, &staging_buffer_create_info ]
    {
      return types::create_unique_buffer( types::get( ctx.device_ ), staging_buffer_create_info );
    };

    std::generate( std::begin( ctx.staging_buffers_ ), std::end( ctx.staging_buffers_ ), create_buffer );

    vkGetBufferMemoryRequirements( types::get( ctx.device_ ),
                                   types::get( ctx.staging_buffers_[ ctx.current_buffer_index_ ] ),
                                   &ctx.staging_memory_requirements_ );

    ctx.staging_aligned_size_ =
      detail::aligned_size( ctx.staging_memory_requirements_.size, ctx.staging_memory_requirements_.alignment );

    auto const memory_type_index =
      detail::find_memory_type( types::get( ctx.physical_device_ ),
                                ctx.staging_memory_requirements_.memoryTypeBits,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    MVK_VERIFY( memory_type_index.value() );

    auto staging_memory_allocate_info = [ &ctx, memory_type_index ]
    {
      auto info            = VkMemoryAllocateInfo();
      info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      info.allocationSize  = context::dynamic_buffer_count * ctx.staging_aligned_size_;
      info.memoryTypeIndex = memory_type_index.value();
      return info;
    }();

    ctx.staging_memory_ = types::create_unique_device_memory( types::get( ctx.device_ ), staging_memory_allocate_info );
    ctx.staging_data_   = detail::map_memory( types::decay( ctx.device_ ),
                                            types::decay( ctx.staging_memory_ ),
                                            context::dynamic_buffer_count * ctx.staging_aligned_size_ );

    for ( size_t i = 0; i < context::dynamic_buffer_count; ++i )
    {
      auto const & current_buffer = ctx.staging_buffers_[ i ];
      vkBindBufferMemory( types::parent( current_buffer ),
                          types::get( current_buffer ),
                          types::get( ctx.staging_memory_ ),
                          i * ctx.staging_aligned_size_ );
    }
  }

  void move_to_garbage_buffers( context & ctx, utility::slice< types::unique_buffer > buffers ) noexcept
  {
    auto & garbage_buffers = ctx.garbage_buffers_[ ctx.current_garbage_index_ ];
    garbage_buffers.reserve( std::size( garbage_buffers ) + std::size( buffers ) );

    for ( auto & buffer : buffers )
    {
      ctx.garbage_buffers_[ ctx.current_garbage_index_ ].push_back( std::move( buffer ) );
    }
  }

  void move_to_garbage_descriptor_sets( context & ctx, utility::slice< types::unique_descriptor_set > sets ) noexcept
  {
    auto & garbage_descriptor_sets = ctx.garbage_descriptor_sets_[ ctx.current_garbage_index_ ];
    garbage_descriptor_sets.reserve( std::size( garbage_descriptor_sets ) + std::size( sets ) );

    for ( auto & set : sets )
    {
      ctx.garbage_descriptor_sets_[ ctx.current_garbage_index_ ].push_back( std::move( set ) );
    }
  }

  void move_to_garbage_memories( context & ctx, types::unique_device_memory memory ) noexcept
  {
    ctx.garbage_memories_[ ctx.current_garbage_index_ ].push_back( std::move( memory ) );
  }

  [[nodiscard]] staging_allocation staging_allocate( context & ctx, utility::slice< std::byte const > src ) noexcept
  {
    ctx.staging_offsets_[ ctx.current_buffer_index_ ] = detail::aligned_size(
      ctx.staging_offsets_[ ctx.current_buffer_index_ ], ctx.staging_memory_requirements_.alignment );

    if ( auto required_size = ctx.staging_offsets_[ ctx.current_buffer_index_ ] + std::size( src );
         required_size > ctx.staging_aligned_size_ )
    {
      // TODO(samuel): should require a move
      move_to_garbage_buffers( ctx, ctx.staging_buffers_ );
      move_to_garbage_memories( ctx, std::move( ctx.staging_memory_ ) );
      create_staging_buffers_and_memories( ctx, required_size * 2 );
      ctx.staging_offsets_[ ctx.current_buffer_index_ ] = 0;
    }

    auto const memory_offset =
      ctx.current_buffer_index_ * ctx.staging_aligned_size_ + ctx.staging_offsets_[ ctx.current_buffer_index_ ];
    auto data = ctx.staging_data_.subslice( memory_offset, std::size( src ) );
    std::copy( std::begin( src ), std::end( src ), std::begin( data ) );

    return { types::decay( ctx.staging_buffers_[ ctx.current_buffer_index_ ] ),
             std::exchange( ctx.staging_offsets_[ ctx.current_buffer_index_ ],
                            ctx.staging_offsets_[ ctx.current_buffer_index_ ] + std::size( src ) ),
             std::size( src ) };
  }

  [[nodiscard]] vertex_allocation vertex_allocate( context & ctx, staging_allocation allocation ) noexcept
  {
    if ( auto required_size = ctx.vertex_offsets_[ ctx.current_buffer_index_ ] + allocation.size_;
         required_size > ctx.vertex_aligned_size_ )
    {
      move_to_garbage_buffers( ctx, ctx.vertex_buffers_ );
      move_to_garbage_memories( ctx, std::move( ctx.vertex_memory_ ) );
      create_vertex_buffers_and_memories( ctx, required_size * 2 );
      ctx.vertex_offsets_[ ctx.current_buffer_index_ ] = 0;
    }

    auto const begin_info = []
    {
      auto info             = VkCommandBufferBeginInfo();
      info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      info.pInheritanceInfo = nullptr;
      return info;
    }();

    auto const copy_region = [ allocation, &ctx ]
    {
      auto region      = VkBufferCopy();
      region.srcOffset = allocation.offset_;
      region.dstOffset = ctx.vertex_offsets_[ ctx.current_buffer_index_ ];
      region.size      = allocation.size_;
      return region;
    }();

    auto const command_buffer = allocate_single_use_command_buffer( ctx );
    vkBeginCommandBuffer( types::get( command_buffer ), &begin_info );

    vkCmdCopyBuffer( types::get( command_buffer ),
                     types::get( allocation.buffer_ ),
                     types::get( ctx.vertex_buffers_[ ctx.current_buffer_index_ ] ),
                     1,
                     &copy_region );

    vkEndCommandBuffer( types::get( command_buffer ) );

    auto submit_info               = VkSubmitInfo();
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &types::get( command_buffer );

    vkQueueSubmit( types::get( ctx.graphics_queue_ ), 1, &submit_info, nullptr );
    vkQueueWaitIdle( types::get( ctx.graphics_queue_ ) );

    return { types::decay( ctx.vertex_buffers_[ ctx.current_buffer_index_ ] ),
             std::exchange( ctx.vertex_offsets_[ ctx.current_buffer_index_ ],
                            ctx.vertex_offsets_[ ctx.current_buffer_index_ ] + allocation.size_ ) };
  }

  [[nodiscard]] index_allocation index_allocate( context & ctx, staging_allocation allocation ) noexcept
  {
    if ( auto required_size = ctx.index_offsets_[ ctx.current_buffer_index_ ] + allocation.size_;
         required_size > ctx.index_aligned_size_ )
    {
      move_to_garbage_buffers( ctx, ctx.index_buffers_ );
      move_to_garbage_memories( ctx, std::move( ctx.index_memory_ ) );
      create_index_buffers_and_memories( ctx, required_size * 2 );
      ctx.index_offsets_[ ctx.current_buffer_index_ ] = 0;
    }

    auto const begin_info = []
    {
      auto info             = VkCommandBufferBeginInfo();
      info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      info.pInheritanceInfo = nullptr;
      return info;
    }();

    auto const copy_region = [ allocation, &ctx ]
    {
      auto region      = VkBufferCopy();
      region.srcOffset = allocation.offset_;
      region.dstOffset = ctx.index_offsets_[ ctx.current_buffer_index_ ];
      region.size      = allocation.size_;
      return region;
    }();

    auto const command_buffer = allocate_single_use_command_buffer( ctx );
    vkBeginCommandBuffer( types::get( command_buffer ), &begin_info );

    vkCmdCopyBuffer( types::get( command_buffer ),
                     types::get( allocation.buffer_ ),
                     types::get( ctx.index_buffers_[ ctx.current_buffer_index_ ] ),
                     1,
                     &copy_region );

    vkEndCommandBuffer( types::get( command_buffer ) );

    auto submit_info               = VkSubmitInfo();
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &types::get( command_buffer );

    vkQueueSubmit( types::get( ctx.graphics_queue_ ), 1, &submit_info, nullptr );
    vkQueueWaitIdle( types::get( ctx.graphics_queue_ ) );

    return { types::decay( ctx.index_buffers_[ ctx.current_buffer_index_ ] ),
             std::exchange( ctx.index_offsets_[ ctx.current_buffer_index_ ],
                            ctx.index_offsets_[ ctx.current_buffer_index_ ] + allocation.size_ ) };
  }

  void create_uniform_buffers_memories_and_sets( context & ctx, types::device_size size ) noexcept
  {
    auto const uniform_buffer_create_info = [ size ]
    {
      auto info        = VkBufferCreateInfo();
      info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      info.size        = size;
      info.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
      info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      return info;
    }();

    auto const create_buffer = [ &ctx, &uniform_buffer_create_info ]
    {
      return types::create_unique_buffer( types::get( ctx.device_ ), uniform_buffer_create_info );
    };

    std::generate( std::begin( ctx.uniform_buffers_ ), std::end( ctx.uniform_buffers_ ), create_buffer );

    vkGetBufferMemoryRequirements( types::get( ctx.device_ ),
                                   types::get( ctx.uniform_buffers_[ ctx.current_buffer_index_ ] ),
                                   &ctx.uniform_memory_requirements_ );

    ctx.uniform_aligned_size_ =
      detail::aligned_size( ctx.uniform_memory_requirements_.size, ctx.uniform_memory_requirements_.alignment );

    auto const memory_type_index =
      detail::find_memory_type( types::get( ctx.physical_device_ ),
                                ctx.uniform_memory_requirements_.memoryTypeBits,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    MVK_VERIFY( memory_type_index.value() );

    auto uniform_memory_allocate_info = [ &ctx, memory_type_index ]
    {
      auto info            = VkMemoryAllocateInfo();
      info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      info.allocationSize  = context::dynamic_buffer_count * ctx.uniform_aligned_size_;
      info.memoryTypeIndex = memory_type_index.value();
      return info;
    }();

    ctx.uniform_memory_ = types::create_unique_device_memory( types::get( ctx.device_ ), uniform_memory_allocate_info );
    ctx.uniform_data_   = detail::map_memory( types::decay( ctx.device_ ),
                                            types::decay( ctx.uniform_memory_ ),
                                            context::dynamic_buffer_count * ctx.uniform_aligned_size_ );
    ctx.uniform_descriptor_sets_ = allocate_descriptor_sets< context::dynamic_buffer_count >(
      ctx, types::decay( ctx.uniform_descriptor_set_layout_ ) );

    for ( size_t i = 0; i < context::dynamic_buffer_count; ++i )
    {
      vkBindBufferMemory( types::parent( ctx.uniform_buffers_[ i ] ),
                          types::get( ctx.uniform_buffers_[ i ] ),
                          types::get( ctx.uniform_memory_ ),
                          i * ctx.uniform_aligned_size_ );

      auto const uniform_descriptor_buffer_info = [ &ctx, i ]
      {
        auto info   = VkDescriptorBufferInfo();
        info.buffer = types::get( ctx.uniform_buffers_[ i ] );
        info.offset = 0;
        info.range  = sizeof( pvm );
        return info;
      }();

      auto const uniform_write = [ &ctx, &uniform_descriptor_buffer_info, i ]
      {
        auto write             = VkWriteDescriptorSet();
        write.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet           = types::get( ctx.uniform_descriptor_sets_[ i ] );
        write.dstBinding       = 0;
        write.dstArrayElement  = 0;
        write.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write.descriptorCount  = 1;
        write.pBufferInfo      = &uniform_descriptor_buffer_info;
        write.pImageInfo       = nullptr;
        write.pTexelBufferView = nullptr;
        return write;
      }();

      auto const descriptor_writes = std::array{ uniform_write };

      vkUpdateDescriptorSets( types::get( ctx.device_ ),
                              static_cast< uint32_t >( std::size( descriptor_writes ) ),
                              std::data( descriptor_writes ),
                              0,
                              nullptr );
    }
  }

  [[nodiscard]] uniform_allocation uniform_allocate( context & ctx, utility::slice< std::byte const > src ) noexcept
  {
    ctx.uniform_offsets_[ ctx.current_buffer_index_ ] =
      detail::aligned_size( ctx.uniform_offsets_[ ctx.current_buffer_index_ ],
                            static_cast< uint32_t >( ctx.uniform_memory_requirements_.alignment ) );

    if ( auto required_size = ctx.uniform_offsets_[ ctx.current_buffer_index_ ] + std::size( src );
         required_size > ctx.uniform_aligned_size_ )
    {
      // TODO(samuel): should require a move
      move_to_garbage_buffers( ctx, ctx.uniform_buffers_ );
      move_to_garbage_descriptor_sets( ctx, ctx.uniform_descriptor_sets_ );

      move_to_garbage_memories( ctx, std::move( ctx.uniform_memory_ ) );
      create_uniform_buffers_memories_and_sets( ctx, required_size * 2 );
      ctx.uniform_offsets_[ ctx.current_buffer_index_ ] = 0;
    }

    auto const memory_offset =
      ctx.current_buffer_index_ * ctx.uniform_aligned_size_ + ctx.uniform_offsets_[ ctx.current_buffer_index_ ];
    auto data = ctx.uniform_data_.subslice( memory_offset, std::size( src ) );
    std::copy( std::begin( src ), std::end( src ), std::begin( data ) );

    return { types::decay( ctx.uniform_descriptor_sets_[ ctx.current_buffer_index_ ] ),
             std::exchange( ctx.uniform_offsets_[ ctx.current_buffer_index_ ],
                            ctx.uniform_offsets_[ ctx.current_buffer_index_ ] + std::size( src ) ) };
  }

  void next_buffer( context & ctx ) noexcept
  {
    ctx.current_buffer_index_ = ( ctx.current_buffer_index_ + 1 ) % context::dynamic_buffer_count;

    ctx.staging_offsets_[ ctx.current_buffer_index_ ] = 0;
    ctx.vertex_offsets_[ ctx.current_buffer_index_ ]  = 0;
    ctx.index_offsets_[ ctx.current_buffer_index_ ]   = 0;
    ctx.uniform_offsets_[ ctx.current_buffer_index_ ] = 0;
  }

}  // namespace mvk::engine
