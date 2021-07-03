#ifndef MVK_ENGINE_ALLOCATE_HPP_INCLUDED
#define MVK_ENGINE_ALLOCATE_HPP_INCLUDED

#include "engine/context.hpp"

namespace mvk::engine
{
  struct staging_allocation
  {
    VkBuffer     buffer_;
    VkDeviceSize offset_;
    VkDeviceSize size_;
  };

  struct vertex_allocation
  {
    VkBuffer     buffer_;
    VkDeviceSize offset_;
  };

  struct index_allocation
  {
    VkBuffer     buffer_;
    VkDeviceSize offset_;
  };

  struct uniform_allocation
  {
    VkDescriptorSet descriptor_set_;
    uint32_t        offset_;
  };

  void create_vertex_buffers_and_memories( context & ctx, VkDeviceSize size ) noexcept;
  void destroy_vertex_buffers_and_memories( context & ctx ) noexcept;
  void create_index_buffers_and_memories( context & ctx, VkDeviceSize size ) noexcept;
  void destroy_index_buffers_and_memories( context & ctx ) noexcept;
  void create_staging_buffers_and_memories( context & ctx, VkDeviceSize size ) noexcept;
  void destroy_staging_buffers_and_memories( context & ctx ) noexcept;
  void create_uniform_buffers_memories_and_sets( context & ctx, VkDeviceSize size ) noexcept;
  void destroy_uniform_buffers_memories_and_sets( context & ctx ) noexcept;

  void destroy_garbage_buffers( context & ctx ) noexcept;
  void destroy_garbage_memories( context & ctx ) noexcept;
  void destroy_garbage_sets( context & ctx ) noexcept;

  void move_to_garbage_buffers( context & ctx, utility::slice< VkBuffer > buffers ) noexcept;
  void move_to_garbage_descriptor_sets( context & ctx, utility::slice< VkDescriptorSet > sets ) noexcept;
  void move_to_garbage_memories( context & ctx, VkDeviceMemory memory ) noexcept;

  [[nodiscard]] staging_allocation staging_allocate( context & ctx, utility::slice< std::byte const > src ) noexcept;
  [[nodiscard]] vertex_allocation  vertex_allocate( context & ctx, staging_allocation allocation ) noexcept;
  [[nodiscard]] index_allocation   index_allocate( context & ctx, staging_allocation allocation ) noexcept;
  [[nodiscard]] uniform_allocation uniform_allocate( context & ctx, utility::slice< std::byte const > src ) noexcept;

  template< size_t Size >
  [[nodiscard]] std::array< VkDescriptorSet, Size > allocate_descriptor_sets( context const &       ctx,
                                                                              VkDescriptorSetLayout layout ) noexcept;

  template< size_t Size >
  [[nodiscard]] std::array< VkCommandBuffer, Size > allocate_command_buffers( context const &      ctx,
                                                                              VkCommandBufferLevel level ) noexcept;

  [[nodiscard]] VkCommandBuffer allocate_single_use_command_buffer( context const & ctx ) noexcept;

  void next_buffer( context & ctx ) noexcept;

}  // namespace mvk::engine

namespace mvk::engine
{
  template< size_t Size >
  [[nodiscard]] std::array< VkDescriptorSet, Size > allocate_descriptor_sets( context const &       ctx,
                                                                              VkDescriptorSetLayout layout ) noexcept
  {
    auto descriptor_set_layouts = std::array< VkDescriptorSetLayout, Size >();
    std::fill( std::begin( descriptor_set_layouts ), std::end( descriptor_set_layouts ), layout );

    auto const allocate_info = [ &ctx, &descriptor_set_layouts ]
    {
      auto info               = VkDescriptorSetAllocateInfo();
      info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      info.descriptorPool     = ctx.descriptor_pool;
      info.descriptorSetCount = static_cast< uint32_t >( std::size( descriptor_set_layouts ) );
      info.pSetLayouts        = std::data( descriptor_set_layouts );
      return info;
    }();

    auto                  descriptor_sets = std::array< VkDescriptorSet, Size >();
    [[maybe_unused]] auto result = vkAllocateDescriptorSets( ctx.device, &allocate_info, std::data( descriptor_sets ) );
    MVK_VERIFY( result == VK_SUCCESS );
    return descriptor_sets;
  }

  template< size_t Size >
  [[nodiscard]] std::array< VkCommandBuffer, Size > allocate_command_buffers( context const &      ctx,
                                                                              VkCommandBufferLevel level ) noexcept
  {
    auto const allocate_info = [ &ctx, level ]
    {
      auto info               = VkCommandBufferAllocateInfo();
      info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      info.commandPool        = ctx.command_pool;
      info.level              = level;
      info.commandBufferCount = Size;
      return info;
    }();

    auto                  command_buffers = std::array< VkCommandBuffer, Size >();
    [[maybe_unused]] auto result = vkAllocateCommandBuffers( ctx.device, &allocate_info, std::data( command_buffers ) );
    MVK_VERIFY( result == VK_SUCCESS );
    return command_buffers;
  }

}  // namespace mvk::engine

#endif
