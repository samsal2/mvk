#ifndef MVK_VK_TYPES_SINGLE_COMMAND_BUFFER_HPP_INCLUDED
#define MVK_VK_TYPES_SINGLE_COMMAND_BUFFER_HPP_INCLUDED

#include "utility/slice.hpp"
#include "vk_types/buffer.hpp"
#include "vk_types/common.hpp"

#include <array>

namespace mvk::vk_types
{

class image;
class pipeline_layout;
class descriptor_sets;
class pipeline;

class single_command_buffer
{
public:
  constexpr single_command_buffer() noexcept = default;

  explicit single_command_buffer(VkCommandBuffer command_buffer);

  single_command_buffer &
  copy_buffer_to_image(
    VkBuffer                          source_buffer,
    VkImage                           destination_image,
    VkImageLayout                     destination_image_layout,
    utility::slice<VkBufferImageCopy> copy_regions) noexcept;

  single_command_buffer &
  copy_buffer(
    VkBuffer                     source_buffer,
    VkBuffer                     destination_buffer,
    utility::slice<VkBufferCopy> copy_regions) noexcept;

  single_command_buffer &
  pipeline_barrier(
    VkPipelineStageFlags                  source_stage,
    VkPipelineStageFlags                  destination_stage,
    VkDependencyFlags                     dependency_flags,
    utility::slice<VkMemoryBarrier>       memory_barriers,
    utility::slice<VkBufferMemoryBarrier> buffer_memory_barriers,
    utility::slice<VkImageMemoryBarrier>  image_memory_barriers) noexcept;

  single_command_buffer &
  bind_descriptor_sets(
    VkPipelineBindPoint bind_point,
    VkPipelineLayout    pipeline_layout,
    uint32_t            descriptor_set_first,
    uint32_t            descriptor_set_count,
    VkDescriptorSet     descriptor_set) noexcept;

  single_command_buffer &
  draw_indexed(
    uint32_t index_count,
    uint32_t instance_count,
    uint32_t first_vertex,
    int32_t  vertex_offset,
    uint32_t first_instance) noexcept;

  single_command_buffer &
  draw(
    uint32_t vertex_count,
    uint32_t instance_count,
    uint32_t first_vertex,
    uint32_t first_instance) noexcept;

  single_command_buffer &
  begin_render_pass(
    VkRenderPassBeginInfo const & begin_info,
    VkSubpassContents             subpass_contents) noexcept;

  single_command_buffer &
  bind_pipeline(VkPipelineBindPoint bind_point, VkPipeline pipeline) noexcept;

  single_command_buffer &
  bind_index_buffer(VkBuffer buffer, VkDeviceSize offset = 0) noexcept;

  single_command_buffer &
  blit_image(
    VkImage                     source_image,
    VkImageLayout               source_image_layout,
    VkImage                     destination_image,
    VkImageLayout               destination_image_layout,
    utility::slice<VkImageBlit> region,
    VkFilter                    filter) noexcept;

  single_command_buffer &
  end_render_pass() noexcept;

  void
  end() noexcept;

  single_command_buffer
  bind_vertex_buffer(
    VkBuffer                     buffer,
    utility::slice<VkDeviceSize> offsets) noexcept;

private:
  VkCommandBuffer command_buffer_ = nullptr;
};

} // namespace mvk::vk_types

#endif
