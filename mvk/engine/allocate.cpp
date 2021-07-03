#include "engine/allocate.hpp"

#include "detail/helpers.hpp"
#include "detail/misc.hpp"
#include "utility/verify.hpp"

#include <vulkan/vulkan.h>

namespace mvk::engine
{
  [[nodiscard]] VkCommandBuffer allocate_single_use_command_buffer( context const & ctx ) noexcept
  {
    auto info               = VkCommandBufferAllocateInfo();
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool        = ctx.command_pool;
    info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = 1;

    auto                  command_buffer = VkCommandBuffer();
    [[maybe_unused]] auto result         = vkAllocateCommandBuffers( ctx.device, &info, &command_buffer );
    MVK_VERIFY( result == VK_SUCCESS );
    return command_buffer;
  }

  void create_vertex_buffers_and_memories( context & ctx, VkDeviceSize size ) noexcept
  {
    auto vertex_buffer_create_info        = VkBufferCreateInfo();
    vertex_buffer_create_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertex_buffer_create_info.size        = size;
    vertex_buffer_create_info.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vertex_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto const create_buffer = [ &ctx, &vertex_buffer_create_info ]
    {
      auto                  buffer = VkBuffer();
      [[maybe_unused]] auto result = vkCreateBuffer( ctx.device, &vertex_buffer_create_info, nullptr, &buffer );
      MVK_VERIFY( result == VK_SUCCESS );
      return buffer;
    };

    std::generate( std::begin( ctx.vertex_buffers ), std::end( ctx.vertex_buffers ), create_buffer );

    vkGetBufferMemoryRequirements(
      ctx.device, ctx.vertex_buffers[ ctx.current_buffer_index ], &ctx.vertex_memory_requirements );

    ctx.vertex_aligned_size =
      detail::aligned_size( ctx.vertex_memory_requirements.size, ctx.vertex_memory_requirements.alignment );

    auto const memory_type_index = detail::find_memory_type(
      ctx.physical_device, ctx.vertex_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    MVK_VERIFY( memory_type_index.has_value() );

    auto vertex_memory_allocate_info            = VkMemoryAllocateInfo();
    vertex_memory_allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vertex_memory_allocate_info.allocationSize  = context::dynamic_buffer_count * ctx.vertex_aligned_size;
    vertex_memory_allocate_info.memoryTypeIndex = memory_type_index.value();

    [[maybe_unused]] auto result =
      vkAllocateMemory( ctx.device, &vertex_memory_allocate_info, nullptr, &ctx.vertex_memory );
    MVK_VERIFY( result == VK_SUCCESS );

    for ( size_t i = 0; i < context::dynamic_buffer_count; ++i )
    {
      vkBindBufferMemory( ctx.device, ctx.vertex_buffers[ i ], ctx.vertex_memory, i * ctx.vertex_aligned_size );
    }
  }

  void create_index_buffers_and_memories( context & ctx, VkDeviceSize size ) noexcept
  {
    auto index_buffer_create_info        = VkBufferCreateInfo();
    index_buffer_create_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    index_buffer_create_info.size        = size;
    index_buffer_create_info.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    index_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto const create_buffer = [ &ctx, &index_buffer_create_info ]
    {
      auto                  buffer = VkBuffer();
      [[maybe_unused]] auto result = vkCreateBuffer( ctx.device, &index_buffer_create_info, nullptr, &buffer );
      MVK_VERIFY( result == VK_SUCCESS );
      return buffer;
    };

    std::generate( std::begin( ctx.index_buffers ), std::end( ctx.index_buffers ), create_buffer );

    vkGetBufferMemoryRequirements(
      ctx.device, ctx.index_buffers[ ctx.current_buffer_index ], &ctx.index_memory_requirements );

    ctx.index_aligned_size =
      detail::aligned_size( ctx.index_memory_requirements.size, ctx.index_memory_requirements.alignment );

    auto const memory_type_index = detail::find_memory_type(
      ctx.physical_device, ctx.index_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    MVK_VERIFY( memory_type_index.has_value() );

    auto index_memory_allocate_info            = VkMemoryAllocateInfo();
    index_memory_allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    index_memory_allocate_info.allocationSize  = context::dynamic_buffer_count * ctx.index_aligned_size;
    index_memory_allocate_info.memoryTypeIndex = memory_type_index.value();

    [[maybe_unused]] auto result =
      vkAllocateMemory( ctx.device, &index_memory_allocate_info, nullptr, &ctx.index_memory );
    MVK_VERIFY( result == VK_SUCCESS );

    for ( size_t i = 0; i < context::dynamic_buffer_count; ++i )
    {
      vkBindBufferMemory( ctx.device, ctx.index_buffers[ i ], ctx.index_memory, i * ctx.index_aligned_size );
    }
  }

  void create_staging_buffers_and_memories( context & ctx, VkDeviceSize size ) noexcept
  {
    auto staging_buffer_create_info        = VkBufferCreateInfo();
    staging_buffer_create_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    staging_buffer_create_info.size        = size;
    staging_buffer_create_info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    staging_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto const create_buffer = [ &ctx, &staging_buffer_create_info ]
    {
      auto                  buffer = VkBuffer();
      [[maybe_unused]] auto result = vkCreateBuffer( ctx.device, &staging_buffer_create_info, nullptr, &buffer );
      MVK_VERIFY( result == VK_SUCCESS );
      return buffer;
    };

    std::generate( std::begin( ctx.staging_buffers ), std::end( ctx.staging_buffers ), create_buffer );

    vkGetBufferMemoryRequirements(
      ctx.device, ctx.staging_buffers[ ctx.current_buffer_index ], &ctx.staging_memory_requirements );

    ctx.staging_aligned_size =
      detail::aligned_size( ctx.staging_memory_requirements.size, ctx.staging_memory_requirements.alignment );

    auto const memory_type_index =
      detail::find_memory_type( ctx.physical_device,
                                ctx.staging_memory_requirements.memoryTypeBits,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    MVK_VERIFY( memory_type_index.value() );

    auto staging_memory_allocate_info            = VkMemoryAllocateInfo();
    staging_memory_allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    staging_memory_allocate_info.allocationSize  = context::dynamic_buffer_count * ctx.staging_aligned_size;
    staging_memory_allocate_info.memoryTypeIndex = memory_type_index.value();

    [[maybe_unused]] auto result =
      vkAllocateMemory( ctx.device, &staging_memory_allocate_info, nullptr, &ctx.staging_memory );
    MVK_VERIFY( result == VK_SUCCESS );

    ctx.staging_data =
      detail::map_memory( ctx.device, ctx.staging_memory, context::dynamic_buffer_count * ctx.staging_aligned_size );

    for ( size_t i = 0; i < context::dynamic_buffer_count; ++i )
    {
      vkBindBufferMemory( ctx.device, ctx.staging_buffers[ i ], ctx.staging_memory, i * ctx.staging_aligned_size );
    }
  }

  void move_to_garbage_buffers( context & ctx, utility::slice< VkBuffer > buffers ) noexcept
  {
    auto & garbage_buffers = ctx.garbage_buffers[ ctx.current_garbage_index ];
    garbage_buffers.reserve( std::size( garbage_buffers ) + std::size( buffers ) );

    for ( auto const buffer : buffers )
    {
      ctx.garbage_buffers[ ctx.current_garbage_index ].push_back( buffer );
    }
  }

  void move_to_garbage_descriptor_sets( context & ctx, utility::slice< VkDescriptorSet > sets ) noexcept
  {
    auto & garbage_descriptor_sets = ctx.garbage_descriptor_sets[ ctx.current_garbage_index ];
    garbage_descriptor_sets.reserve( std::size( garbage_descriptor_sets ) + std::size( sets ) );

    for ( auto const set : sets )
    {
      ctx.garbage_descriptor_sets[ ctx.current_garbage_index ].push_back( set );
    }
  }

  void move_to_garbage_memories( context & ctx, VkDeviceMemory memory ) noexcept
  {
    ctx.garbage_memories[ ctx.current_garbage_index ].push_back( memory );
  }

  [[nodiscard]] staging_allocation staging_allocate( context & ctx, utility::slice< std::byte const > src ) noexcept
  {
    ctx.staging_offsets[ ctx.current_buffer_index ] = detail::aligned_size(
      ctx.staging_offsets[ ctx.current_buffer_index ], ctx.staging_memory_requirements.alignment );

    if ( auto required_size = ctx.staging_offsets[ ctx.current_buffer_index ] + std::size( src );
         required_size > ctx.staging_aligned_size )
    {
      move_to_garbage_buffers( ctx, ctx.staging_buffers );
      move_to_garbage_memories( ctx, ctx.staging_memory );
      create_staging_buffers_and_memories( ctx, required_size * 2 );
      ctx.staging_offsets[ ctx.current_buffer_index ] = 0;
    }

    auto const memory_offset =
      ctx.current_buffer_index * ctx.staging_aligned_size + ctx.staging_offsets[ ctx.current_buffer_index ];
    auto data = ctx.staging_data.subslice( memory_offset, std::size( src ) );
    std::copy( std::begin( src ), std::end( src ), std::begin( data ) );

    return { ctx.staging_buffers[ ctx.current_buffer_index ],
             std::exchange( ctx.staging_offsets[ ctx.current_buffer_index ],
                            ctx.staging_offsets[ ctx.current_buffer_index ] + std::size( src ) ),
             std::size( src ) };
  }

  [[nodiscard]] vertex_allocation vertex_allocate( context & ctx, staging_allocation allocation ) noexcept
  {
    if ( auto required_size = ctx.vertex_offsets[ ctx.current_buffer_index ] + allocation.size_;
         required_size > ctx.vertex_aligned_size )
    {
      move_to_garbage_buffers( ctx, ctx.vertex_buffers );
      move_to_garbage_memories( ctx, ctx.vertex_memory );
      create_vertex_buffers_and_memories( ctx, required_size * 2 );
      ctx.vertex_offsets[ ctx.current_buffer_index ] = 0;
    }

    auto command_buffer_begin_info             = VkCommandBufferBeginInfo();
    command_buffer_begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;

    auto copy_region      = VkBufferCopy();
    copy_region.srcOffset = allocation.offset_;
    copy_region.dstOffset = ctx.vertex_offsets[ ctx.current_buffer_index ];
    copy_region.size      = allocation.size_;

    auto const command_buffer = allocate_single_use_command_buffer( ctx );
    vkBeginCommandBuffer( command_buffer, &command_buffer_begin_info );

    vkCmdCopyBuffer(
      command_buffer, allocation.buffer_, ctx.vertex_buffers[ ctx.current_buffer_index ], 1, &copy_region );

    vkEndCommandBuffer( command_buffer );

    auto submit_info               = VkSubmitInfo();
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &command_buffer;

    vkQueueSubmit( ctx.graphics_queue, 1, &submit_info, nullptr );
    vkQueueWaitIdle( ctx.graphics_queue );
    vkFreeCommandBuffers( ctx.device, ctx.command_pool, 1, &command_buffer );

    return { ctx.vertex_buffers[ ctx.current_buffer_index ],
             std::exchange( ctx.vertex_offsets[ ctx.current_buffer_index ],
                            ctx.vertex_offsets[ ctx.current_buffer_index ] + allocation.size_ ) };
  }

  [[nodiscard]] index_allocation index_allocate( context & ctx, staging_allocation allocation ) noexcept
  {
    if ( auto required_size = ctx.index_offsets[ ctx.current_buffer_index ] + allocation.size_;
         required_size > ctx.index_aligned_size )
    {
      move_to_garbage_buffers( ctx, ctx.index_buffers );
      move_to_garbage_memories( ctx, ctx.index_memory );
      create_index_buffers_and_memories( ctx, required_size * 2 );
      ctx.index_offsets[ ctx.current_buffer_index ] = 0;
    }

    auto command_buffer_begin_info             = VkCommandBufferBeginInfo();
    command_buffer_begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;

    auto copy_region      = VkBufferCopy();
    copy_region.srcOffset = allocation.offset_;
    copy_region.dstOffset = ctx.index_offsets[ ctx.current_buffer_index ];
    copy_region.size      = allocation.size_;

    auto const command_buffer = allocate_single_use_command_buffer( ctx );
    vkBeginCommandBuffer( command_buffer, &command_buffer_begin_info );

    vkCmdCopyBuffer(
      command_buffer, allocation.buffer_, ctx.index_buffers[ ctx.current_buffer_index ], 1, &copy_region );

    vkEndCommandBuffer( command_buffer );

    auto submit_info               = VkSubmitInfo();
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &command_buffer;

    vkQueueSubmit( ctx.graphics_queue, 1, &submit_info, nullptr );
    vkQueueWaitIdle( ctx.graphics_queue );
    vkFreeCommandBuffers( ctx.device, ctx.command_pool, 1, &command_buffer );

    return { ctx.index_buffers[ ctx.current_buffer_index ],
             std::exchange( ctx.index_offsets[ ctx.current_buffer_index ],
                            ctx.index_offsets[ ctx.current_buffer_index ] + allocation.size_ ) };
  }

  void create_uniform_buffers_memories_and_sets( context & ctx, VkDeviceSize size ) noexcept
  {
    auto uniform_buffer_create_info        = VkBufferCreateInfo();
    uniform_buffer_create_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    uniform_buffer_create_info.size        = size;
    uniform_buffer_create_info.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    uniform_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto const create_buffer = [ &ctx, &uniform_buffer_create_info ]
    {
      auto                  buffer = VkBuffer();
      [[maybe_unused]] auto result = vkCreateBuffer( ctx.device, &uniform_buffer_create_info, nullptr, &buffer );
      MVK_VERIFY( result == VK_SUCCESS );
      return buffer;
    };

    std::generate( std::begin( ctx.uniform_buffers ), std::end( ctx.uniform_buffers ), create_buffer );

    vkGetBufferMemoryRequirements(
      ctx.device, ctx.uniform_buffers[ ctx.current_buffer_index ], &ctx.uniform_memory_requirements );

    ctx.uniform_aligned_size =
      detail::aligned_size( ctx.uniform_memory_requirements.size, ctx.uniform_memory_requirements.alignment );

    auto const memory_type_index =
      detail::find_memory_type( ctx.physical_device,
                                ctx.uniform_memory_requirements.memoryTypeBits,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    MVK_VERIFY( memory_type_index.value() );

    auto uniform_memory_allocate_info            = VkMemoryAllocateInfo();
    uniform_memory_allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    uniform_memory_allocate_info.allocationSize  = context::dynamic_buffer_count * ctx.uniform_aligned_size;
    uniform_memory_allocate_info.memoryTypeIndex = memory_type_index.value();

    [[maybe_unused]] auto result =
      vkAllocateMemory( ctx.device, &uniform_memory_allocate_info, nullptr, &ctx.uniform_memory );
    MVK_VERIFY( result == VK_SUCCESS );

    ctx.uniform_data =
      detail::map_memory( ctx.device, ctx.uniform_memory, context::dynamic_buffer_count * ctx.uniform_aligned_size );

    ctx.uniform_descriptor_sets =
      allocate_descriptor_sets< context::dynamic_buffer_count >( ctx, ctx.uniform_descriptor_set_layout );

    for ( size_t i = 0; i < context::dynamic_buffer_count; ++i )
    {
      vkBindBufferMemory( ctx.device, ctx.uniform_buffers[ i ], ctx.uniform_memory, i * ctx.uniform_aligned_size );

      auto uniform_descriptor_buffer_info   = VkDescriptorBufferInfo();
      uniform_descriptor_buffer_info.buffer = ctx.uniform_buffers[ i ];
      uniform_descriptor_buffer_info.offset = 0;
      uniform_descriptor_buffer_info.range  = sizeof( pvm );

      auto uniform_descriptor_write_set             = VkWriteDescriptorSet();
      uniform_descriptor_write_set.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      uniform_descriptor_write_set.dstSet           = ctx.uniform_descriptor_sets[ i ];
      uniform_descriptor_write_set.dstBinding       = 0;
      uniform_descriptor_write_set.dstArrayElement  = 0;
      uniform_descriptor_write_set.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
      uniform_descriptor_write_set.descriptorCount  = 1;
      uniform_descriptor_write_set.pBufferInfo      = &uniform_descriptor_buffer_info;
      uniform_descriptor_write_set.pImageInfo       = nullptr;
      uniform_descriptor_write_set.pTexelBufferView = nullptr;

      vkUpdateDescriptorSets( ctx.device, 1, &uniform_descriptor_write_set, 0, nullptr );
    }
  }

  [[nodiscard]] uniform_allocation uniform_allocate( context & ctx, utility::slice< std::byte const > src ) noexcept
  {
    ctx.uniform_offsets[ ctx.current_buffer_index ] =
      detail::aligned_size( ctx.uniform_offsets[ ctx.current_buffer_index ],
                            static_cast< uint32_t >( ctx.uniform_memory_requirements.alignment ) );

    if ( auto required_size = ctx.uniform_offsets[ ctx.current_buffer_index ] + std::size( src );
         required_size > ctx.uniform_aligned_size )
    {
      move_to_garbage_buffers( ctx, ctx.uniform_buffers );
      move_to_garbage_descriptor_sets( ctx, ctx.uniform_descriptor_sets );
      move_to_garbage_memories( ctx, ctx.uniform_memory );
      create_uniform_buffers_memories_and_sets( ctx, required_size * 2 );
      ctx.uniform_offsets[ ctx.current_buffer_index ] = 0;
    }

    auto const memory_offset =
      ctx.current_buffer_index * ctx.uniform_aligned_size + ctx.uniform_offsets[ ctx.current_buffer_index ];
    auto data = ctx.uniform_data.subslice( memory_offset, std::size( src ) );
    std::copy( std::begin( src ), std::end( src ), std::begin( data ) );

    return { ctx.uniform_descriptor_sets[ ctx.current_buffer_index ],
             std::exchange( ctx.uniform_offsets[ ctx.current_buffer_index ],
                            ctx.uniform_offsets[ ctx.current_buffer_index ] + std::size( src ) ) };
  }

  void destroy_vertex_buffers_and_memories( context & ctx ) noexcept
  {
    vkFreeMemory( ctx.device, ctx.vertex_memory, nullptr );
    for ( auto const buffer : ctx.vertex_buffers )
    {
      vkDestroyBuffer( ctx.device, buffer, nullptr );
    }
  }

  void destroy_index_buffers_and_memories( context & ctx ) noexcept
  {
    vkFreeMemory( ctx.device, ctx.index_memory, nullptr );
    for ( auto const buffer : ctx.index_buffers )
    {
      vkDestroyBuffer( ctx.device, buffer, nullptr );
    }
  }

  void destroy_staging_buffers_and_memories( context & ctx ) noexcept
  {
    vkFreeMemory( ctx.device, ctx.staging_memory, nullptr );
    for ( auto const buffer : ctx.staging_buffers )
    {
      vkDestroyBuffer( ctx.device, buffer, nullptr );
    }
  }

  void destroy_uniform_buffers_memories_and_sets( context & ctx ) noexcept
  {
    vkFreeDescriptorSets( ctx.device,
                          ctx.descriptor_pool,
                          static_cast< uint32_t >( std::size( ctx.uniform_descriptor_sets ) ),
                          std::data( ctx.uniform_descriptor_sets ) );

    vkFreeMemory( ctx.device, ctx.uniform_memory, nullptr );
    for ( auto const buffer : ctx.uniform_buffers )
    {
      vkDestroyBuffer( ctx.device, buffer, nullptr );
    }
  }

  void destroy_garbage_buffers( context & ctx ) noexcept
  {
    for ( auto const & buffers : ctx.garbage_buffers )
    {
      for ( auto const buffer : buffers )
      {
        vkDestroyBuffer( ctx.device, buffer, nullptr );
      }
    }
  }

  void destroy_garbage_memories( context & ctx ) noexcept
  {
    {
      for ( auto const & memories : ctx.garbage_memories )
      {
        for ( auto const memory : memories )
        {
          vkFreeMemory( ctx.device, memory, nullptr );
        }
      }
    }
  }
  void destroy_garbage_sets( context & ctx ) noexcept
  {
    for ( auto const & sets : ctx.garbage_descriptor_sets )
    {
      for ( auto const set : sets )
      {
        vkFreeDescriptorSets( ctx.device, ctx.descriptor_pool, 1, &set );
      }
    }
  }

  void next_buffer( context & ctx ) noexcept
  {
    ctx.current_buffer_index = ( ctx.current_buffer_index + 1 ) % context::dynamic_buffer_count;

    ctx.staging_offsets[ ctx.current_buffer_index ] = 0;
    ctx.vertex_offsets[ ctx.current_buffer_index ]  = 0;
    ctx.index_offsets[ ctx.current_buffer_index ]   = 0;
    ctx.uniform_offsets[ ctx.current_buffer_index ] = 0;
  }

}  // namespace mvk::engine
