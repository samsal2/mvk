#ifndef MVK_ENGINE_ALLOCATE_HPP_INCLUDED
#define MVK_ENGINE_ALLOCATE_HPP_INCLUDED

#include "engine/context.hpp"

namespace mvk::engine
{
  struct staging_allocation
  {
    types::buffer      buffer_;
    types::device_size offset_;
    types::device_size size_;
  };

  struct vertex_allocation
  {
    types::buffer      buffer_;
    types::device_size offset_;
  };

  struct index_allocation
  {
    types::buffer      buffer_;
    types::device_size offset_;
  };

  struct uniform_allocation
  {
    types::descriptor_set descriptor_set_;
    uint32_t              offset_;
  };

  void create_vertex_buffers_and_memories( context & ctx, types::device_size size ) noexcept;
  void create_index_buffers_and_memories( context & ctx, types::device_size size ) noexcept;
  void create_staging_buffers_and_memories( context & ctx, types::device_size size ) noexcept;
  void create_uniform_buffers_memories_and_sets( context & ctx, types::device_size size ) noexcept;

  void move_to_garbage_buffers( context & ctx, utility::slice< types::unique_buffer > buffers ) noexcept;
  void move_to_garbage_descriptor_sets( context & ctx, utility::slice< types::unique_descriptor_set > sets ) noexcept;
  void move_to_garbage_memories( context & ctx, types::unique_device_memory memory ) noexcept;

  [[nodiscard]] staging_allocation staging_allocate( context & ctx, utility::slice< std::byte const > src ) noexcept;
  [[nodiscard]] vertex_allocation  vertex_allocate( context & ctx, staging_allocation allocation ) noexcept;
  [[nodiscard]] index_allocation   index_allocate( context & ctx, staging_allocation allocation ) noexcept;
  [[nodiscard]] uniform_allocation uniform_allocate( context & ctx, utility::slice< std::byte const > src ) noexcept;

  template< size_t Size >
  [[nodiscard]] std::array< types::unique_descriptor_set, Size >
    allocate_descriptor_sets( context const & ctx, types::descriptor_set_layout layout ) noexcept;

  template< size_t Size >
  [[nodiscard]] std::array< types::unique_command_buffer, Size >
    allocate_command_buffers( context const & ctx, VkCommandBufferLevel level ) noexcept;

  [[nodiscard]] types::unique_command_buffer allocate_single_use_command_buffer( context const & ctx ) noexcept;

  void next_buffer( context & ctx ) noexcept;

}  // namespace mvk::engine

namespace mvk::engine
{
  template< size_t Size >
  [[nodiscard]] std::array< types::unique_descriptor_set, Size >
    allocate_descriptor_sets( context const & ctx, types::descriptor_set_layout layout ) noexcept
  {
    auto descriptor_set_layouts = std::array< VkDescriptorSetLayout, Size >();
    std::fill( std::begin( descriptor_set_layouts ), std::end( descriptor_set_layouts ), types::get( layout ) );

    auto const allocate_info = [ &ctx, &descriptor_set_layouts ]
    {
      auto info               = VkDescriptorSetAllocateInfo();
      info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      info.descriptorPool     = types::get( ctx.descriptor_pool_ );
      info.descriptorSetCount = static_cast< uint32_t >( std::size( descriptor_set_layouts ) );
      info.pSetLayouts        = std::data( descriptor_set_layouts );
      return info;
    }();

    auto handles = std::array< VkDescriptorSet, Size >();

    [[maybe_unused]] auto result =
      vkAllocateDescriptorSets( types::get( ctx.device_ ), &allocate_info, std::data( handles ) );

    MVK_VERIFY( result == VK_SUCCESS );

    auto descriptor_sets = std::array< types::unique_descriptor_set, Size >();

    for ( auto i = size_t( 0 ); i < Size; ++i )
    {
      descriptor_sets[ i ] =
        types::unique_descriptor_set( handles[ i ], types::get( ctx.device_ ), types::get( ctx.descriptor_pool_ ) );
    }

    return descriptor_sets;
  }

  template< size_t Size >
  [[nodiscard]] std::array< types::unique_command_buffer, Size >
    allocate_command_buffers( context const & ctx, VkCommandBufferLevel level ) noexcept
  {
    auto const allocate_info = [ &ctx, level ]
    {
      auto info               = VkCommandBufferAllocateInfo();
      info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      info.commandPool        = types::get( ctx.command_pool_ );
      info.level              = level;
      info.commandBufferCount = Size;
      return info;
    }();

    auto handles = std::array< VkCommandBuffer, Size >();

    [[maybe_unused]] auto result =
      vkAllocateCommandBuffers( types::get( ctx.device_ ), &allocate_info, std::data( handles ) );

    MVK_VERIFY( result == VK_SUCCESS );

    auto command_buffers = std::array< types::unique_command_buffer, Size >();

    for ( auto i = size_t( 0 ); i < Size; ++i )
    {
      command_buffers[ i ] =
        types::unique_command_buffer( handles[ i ], types::get( ctx.device_ ), types::get( ctx.command_pool_ ) );
    }

    return command_buffers;
  }

}  // namespace mvk::engine

#endif
