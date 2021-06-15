#include "types/single_command_buffer.hpp"

#include "utility/misc.hpp"
#include "utility/types.hpp"

namespace mvk::types
{

single_command_buffer::single_command_buffer(
    VkCommandBuffer const command_buffer) noexcept
    : command_buffer_(command_buffer)
{
}

single_command_buffer &
single_command_buffer::copy_buffer_to_image(
    copy_buffer_to_image_information information,
    utility::slice<VkBufferImageCopy> copy_regions) noexcept
{
  auto const [source_buffer, destination_image, destination_image_layout] =
      information;
  auto const [copy_regions_data, copy_regions_size] =
      utility::bind_data_and_size(copy_regions);

  vkCmdCopyBufferToImage(command_buffer_, source_buffer, destination_image,
                         destination_image_layout,
                         static_cast<uint32_t>(copy_regions_size),
                         copy_regions_data);

  return *this;
}

single_command_buffer &
single_command_buffer::copy_buffer(
    copy_buffer_information const information,
    utility::slice<VkBufferCopy> copy_regions) noexcept
{
  auto const [source_buffer, destination_buffer] = information;
  auto const [copy_regions_data, copy_regions_size] =
      utility::bind_data_and_size(copy_regions);

  vkCmdCopyBuffer(command_buffer_, source_buffer, destination_buffer,
                  static_cast<uint32_t>(copy_regions_size),
                  copy_regions_data);
  return *this;
}

single_command_buffer &
single_command_buffer::pipeline_barrier(
    pipeline_barrier_information information,
    utility::slice<VkMemoryBarrier> memory_barriers,
    utility::slice<VkBufferMemoryBarrier> buffer_memory_barriers,
    utility::slice<VkImageMemoryBarrier> image_memory_barriers) noexcept
{
  auto const [source_stage, destination_stage, dependency_flags] =
      information;
  auto const [memory_barriers_data, memory_barriers_size] =
      utility::bind_data_and_size(memory_barriers);
  auto const [buffer_memory_barriers_data, buffer_memory_barriers_size] =
      utility::bind_data_and_size(buffer_memory_barriers);
  auto const [image_memory_barriers_data, image_memory_barriers_size] =
      utility::bind_data_and_size(image_memory_barriers);

  vkCmdPipelineBarrier(
      command_buffer_, source_stage, destination_stage, dependency_flags,
      static_cast<uint32_t>(memory_barriers_size), memory_barriers_data,
      static_cast<uint32_t>(buffer_memory_barriers_size),
      buffer_memory_barriers_data,
      static_cast<uint32_t>(image_memory_barriers_size),
      image_memory_barriers_data);
  return *this;
}

single_command_buffer &
single_command_buffer::begin_render_pass(
    VkRenderPassBeginInfo const & begin_info,
    VkSubpassContents const subpass_contents) noexcept
{
  vkCmdBeginRenderPass(command_buffer_, &begin_info, subpass_contents);
  return *this;
}

single_command_buffer &
single_command_buffer::bind_pipeline(VkPipelineBindPoint const bind_point,
                                     VkPipeline const pipeline) noexcept
{
  vkCmdBindPipeline(command_buffer_, bind_point, pipeline);
  return *this;
}

single_command_buffer &
single_command_buffer::bind_index_buffer(VkBuffer const buffer,
                                         device_size const offset) noexcept
{
  vkCmdBindIndexBuffer(command_buffer_, buffer, offset, VK_INDEX_TYPE_UINT32);
  return *this;
}

single_command_buffer &
single_command_buffer::bind_descriptor_sets(
    VkPipelineBindPoint const bind_point,
    VkPipelineLayout const pipeline_layout,
    uint32_t const descriptor_set_first, uint32_t const descriptor_set_count,
    VkDescriptorSet const descriptor_set) noexcept
{
  vkCmdBindDescriptorSets(command_buffer_, bind_point, pipeline_layout,
                          descriptor_set_first, descriptor_set_count,
                          &descriptor_set, 0, nullptr);
  return *this;
}

single_command_buffer &
single_command_buffer::draw(uint32_t const vertex_count,
                            uint32_t const instance_count,
                            uint32_t const first_vertex,
                            uint32_t const first_instance) noexcept
{
  vkCmdDraw(command_buffer_, vertex_count, instance_count, first_vertex,
            first_instance);
  return *this;
}

single_command_buffer &
single_command_buffer::draw_indexed(uint32_t const index_count,
                                    uint32_t const instance_count,
                                    uint32_t const first_vertex,
                                    int32_t const vertex_offset,
                                    uint32_t const first_instance) noexcept
{
  vkCmdDrawIndexed(command_buffer_, index_count, instance_count, first_vertex,
                   vertex_offset, first_instance);
  return *this;
}

single_command_buffer &
single_command_buffer::blit_image(blit_image_information source,
                                  blit_image_information destination,
                                  utility::slice<VkImageBlit> regions,
                                  VkFilter filter) noexcept
{
  auto const [source_image, source_image_layout] = source;
  auto const [destination_image, destination_image_layout] = destination;
  auto const [regions_data, regions_size] =
      utility::bind_data_and_size(regions);

  vkCmdBlitImage(command_buffer_, source_image, source_image_layout,
                 destination_image, destination_image_layout,
                 static_cast<uint32_t>(regions_size), regions_data, filter);

  return *this;
}

single_command_buffer &
single_command_buffer::end_render_pass() noexcept
{
  vkCmdEndRenderPass(command_buffer_);
  return *this;
}

void
single_command_buffer::end() noexcept
{
  vkEndCommandBuffer(command_buffer_);
}

single_command_buffer
single_command_buffer::bind_vertex_buffer(
    utility::slice<VkBuffer> const buffers,
    utility::slice<device_size> const offsets) noexcept
{
  auto const [buffers_data, buffers_size] =
      utility::bind_data_and_size(buffers);
  vkCmdBindVertexBuffers(command_buffer_, 0,
                         static_cast<uint32_t>(buffers_size), buffers_data,
                         std::data(offsets));
  return *this;
}

} // namespace mvk::types
