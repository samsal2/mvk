#ifndef MVK_TYPES_SINGLE_COMMAND_BUFFER_HPP_INCLUDED
#define MVK_TYPES_SINGLE_COMMAND_BUFFER_HPP_INCLUDED

#include "types/buffer.hpp"
#include "types/common.hpp"
#include "utility/slice.hpp"

#include <array>

namespace mvk::types
{

class image;
class pipeline_layout;
class descriptor_sets;
class pipeline;

class single_command_buffer
{
public:
  constexpr single_command_buffer() noexcept = default;

  explicit single_command_buffer(VkCommandBuffer command_buffer) noexcept;

  struct copy_buffer_to_image_information
  {
    VkBuffer source_buffer_;
    VkImage destination_image_;
    VkImageLayout destination_image_layout_;
  };

  single_command_buffer &
  copy_buffer_to_image(
      copy_buffer_to_image_information information,
      utility::slice<VkBufferImageCopy> copy_regions) noexcept;

  struct copy_buffer_information
  {
    VkBuffer source_buffer_;
    VkBuffer destination_buffer_;
  };

  single_command_buffer &
  copy_buffer(copy_buffer_information information,
              utility::slice<VkBufferCopy> copy_regions) noexcept;

  struct pipeline_barrier_information
  {
    VkPipelineStageFlags source_stage_;
    VkPipelineStageFlags destination_stage_;
    VkDependencyFlags dependency_flags_;
  };

  single_command_buffer &
  pipeline_barrier(
      pipeline_barrier_information pipeline_barrier_information,
      utility::slice<VkMemoryBarrier> memory_barriers,
      utility::slice<VkBufferMemoryBarrier> buffer_memory_barriers,
      utility::slice<VkImageMemoryBarrier> image_memory_barriers) noexcept;

  struct bind_descriptor_sets_information
  {
    VkPipelineBindPoint bind_point_;
    VkPipelineLayout pipeline_layout_;
    uint32_t descriptor_set_first_;
    uint32_t descriptor_set_count_;
  };

  single_command_buffer &
  bind_descriptor_sets(
      bind_descriptor_sets_information bind_descriptor_sets_information,
      utility::slice<VkDescriptorSet> descriptor_sets,
      utility::slice<uint32_t> dynamic_offsets) noexcept;

  single_command_buffer &
  draw_indexed(uint32_t index_count, uint32_t instance_count,
               uint32_t first_vertex, int32_t vertex_offset,
               uint32_t first_instance) noexcept;

  single_command_buffer &
  draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex,
       uint32_t first_instance) noexcept;

  single_command_buffer &
  begin_render_pass(VkRenderPassBeginInfo const & begin_info,
                    VkSubpassContents subpass_contents) noexcept;

  single_command_buffer &
  bind_pipeline(VkPipelineBindPoint bind_point, VkPipeline pipeline) noexcept;

  single_command_buffer &
  bind_index_buffer(VkBuffer buffer, device_size offset = 0) noexcept;

  struct blit_image_information
  {
    VkImage image_;
    VkImageLayout image_layout_;
  };

  single_command_buffer &
  blit_image(blit_image_information source,
             blit_image_information destination,
             utility::slice<VkImageBlit> regions, VkFilter filter) noexcept;

  single_command_buffer &
  end_render_pass() noexcept;

  void
  end() noexcept;

  single_command_buffer
  bind_vertex_buffer(utility::slice<VkBuffer> buffers,
                     utility::slice<device_size> offsets) noexcept;

  [[nodiscard]] constexpr VkCommandBuffer
  get() const noexcept;

private:
  VkCommandBuffer command_buffer_ = nullptr;
};

[[nodiscard]] constexpr VkCommandBuffer
single_command_buffer::get() const noexcept
{
  return command_buffer_;
}

} // namespace mvk::types

#endif
