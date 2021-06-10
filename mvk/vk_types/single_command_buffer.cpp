#include "vk_types/single_command_buffer.hpp"

#include "utility/types.hpp"

namespace mvk::vk_types
{

single_command_buffer::single_command_buffer(VkCommandBuffer const command_buffer) : command_buffer_(command_buffer)
{
}

single_command_buffer &
single_command_buffer::copy_buffer_to_image(VkBuffer const source_buffer, VkImage const destination_image, VkImageLayout const destination_image_layout,
                                            utility::slice<VkBufferImageCopy> const copy_regions) noexcept
{
        auto const [copy_regions_data, copy_regions_size] = utility::bind_data_and_size(copy_regions);

        vkCmdCopyBufferToImage(command_buffer_, source_buffer, destination_image, destination_image_layout, static_cast<uint32_t>(copy_regions_size),
                               copy_regions_data);

        return *this;
}

single_command_buffer &
single_command_buffer::copy_buffer(VkBuffer const source_buffer, VkBuffer const destination_buffer, utility::slice<VkBufferCopy> const copy_regions) noexcept
{
        auto const [copy_regions_data, copy_regions_size] = utility::bind_data_and_size(copy_regions);

        vkCmdCopyBuffer(command_buffer_, source_buffer, destination_buffer, static_cast<uint32_t>(copy_regions_size), copy_regions_data);
        return *this;
}

single_command_buffer &
single_command_buffer::pipeline_barrier(VkPipelineStageFlags const source_stage, VkPipelineStageFlags const destination_stage,
                                        VkDependencyFlags const dependency_flags, utility::slice<VkMemoryBarrier> memory_barriers,
                                        utility::slice<VkBufferMemoryBarrier> buffer_memory_barriers,
                                        utility::slice<VkImageMemoryBarrier>  image_memory_barriers) noexcept
{
        auto const [memory_barriers_data, memory_barriers_size]               = utility::bind_data_and_size(memory_barriers);
        auto const [buffer_memory_barriers_data, buffer_memory_barriers_size] = utility::bind_data_and_size(buffer_memory_barriers);
        auto const [image_memory_barriers_data, image_memory_barriers_size]   = utility::bind_data_and_size(image_memory_barriers);

        vkCmdPipelineBarrier(command_buffer_, source_stage, destination_stage, dependency_flags, static_cast<uint32_t>(memory_barriers_size),
                             memory_barriers_data, static_cast<uint32_t>(buffer_memory_barriers_size), buffer_memory_barriers_data,
                             static_cast<uint32_t>(image_memory_barriers_size), image_memory_barriers_data);
        return *this;
}

single_command_buffer &
single_command_buffer::begin_render_pass(VkRenderPassBeginInfo const & begin_info, VkSubpassContents const subpass_contents) noexcept
{
        vkCmdBeginRenderPass(command_buffer_, &begin_info, subpass_contents);
        return *this;
}

single_command_buffer &
single_command_buffer::bind_pipeline(VkPipelineBindPoint const bind_point, VkPipeline const pipeline) noexcept
{
        vkCmdBindPipeline(command_buffer_, bind_point, pipeline);
        return *this;
}

single_command_buffer &
single_command_buffer::bind_index_buffer(VkBuffer const buffer, VkDeviceSize const offset) noexcept
{
        vkCmdBindIndexBuffer(command_buffer_, buffer, offset, VK_INDEX_TYPE_UINT32);
        return *this;
}

single_command_buffer &
single_command_buffer::bind_descriptor_sets(VkPipelineBindPoint const bind_point, VkPipelineLayout const pipeline_layout, uint32_t const descriptor_set_first,
                                            uint32_t const descriptor_set_count, VkDescriptorSet const descriptor_set) noexcept
{
        vkCmdBindDescriptorSets(command_buffer_, bind_point, pipeline_layout, descriptor_set_first, descriptor_set_count, &descriptor_set, 0, nullptr);
        return *this;
}

single_command_buffer &
single_command_buffer::draw(uint32_t const vertex_count, uint32_t const instance_count, uint32_t const first_vertex, uint32_t const first_instance) noexcept
{
        vkCmdDraw(command_buffer_, vertex_count, instance_count, first_vertex, first_instance);
        return *this;
}

single_command_buffer &
single_command_buffer::draw_indexed(uint32_t const index_count, uint32_t const instance_count, uint32_t const first_vertex, int32_t const vertex_offset,
                                    uint32_t const first_instance) noexcept
{
        vkCmdDrawIndexed(command_buffer_, index_count, instance_count, first_vertex, vertex_offset, first_instance);
        return *this;
}

single_command_buffer &
single_command_buffer::blit_image(VkImage const source_image, VkImageLayout const source_image_layout, VkImage const destination_image,
                                  VkImageLayout const destination_image_layout, utility::slice<VkImageBlit> const regions, VkFilter const filter) noexcept
{
        auto const [regions_data, regions_size] = utility::bind_data_and_size(regions);

        vkCmdBlitImage(command_buffer_, source_image, source_image_layout, destination_image, destination_image_layout, static_cast<uint32_t>(regions_size),
                       regions_data, filter);

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
single_command_buffer::bind_vertex_buffer(VkBuffer const buffer, utility::slice<VkDeviceSize> const offsets) noexcept
{
        auto const buffers      = std::array{buffer};
        auto const buffers_size = static_cast<uint32_t>(std::size(buffers));

        vkCmdBindVertexBuffers(command_buffer_, 0, buffers_size, std::data(buffers), std::data(offsets));
        return *this;
}

} // namespace mvk::vk_types
