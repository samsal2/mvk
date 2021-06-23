#include "detail/misc.hpp"

#include "detail/creators.hpp"
#include "vulkan/vulkan_core.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma clang diagnostic pop

#include <array>
#include <cmath>
#include <optional>
#include <utility>

namespace mvk::detail
{
void
submit_draw_commands(types::device const device,
                     types::queue const graphics_queue,
                     types::command_buffer const command_buffer,
                     types::semaphore const image_available,
                     types::semaphore const render_finished,
                     types::fence const frame_in_flight_fence) noexcept
{
  auto const wait_semaphores = std::array{types::get(image_available)};
  auto const signal_semaphores = std::array{types::get(render_finished)};
  auto const wait_stages = std::array<VkPipelineStageFlags, 1>{
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  auto const command_buffers = std::array{types::get(command_buffer)};

  auto const submit_info =
      [&wait_semaphores, &signal_semaphores, &wait_stages, &command_buffers]
  {
    auto info = VkSubmitInfo();
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.waitSemaphoreCount =
        static_cast<uint32_t>(std::size(wait_semaphores));
    info.pWaitSemaphores = std::data(wait_semaphores);
    info.pWaitDstStageMask = std::data(wait_stages);
    info.commandBufferCount =
        static_cast<uint32_t>(std::size(command_buffers));
    info.pCommandBuffers = std::data(command_buffers);
    info.signalSemaphoreCount =
        static_cast<uint32_t>(std::size(signal_semaphores));
    info.pSignalSemaphores = std::data(signal_semaphores);
    return info;
  }();

  vkResetFences(types::get(device), 1, &types::get(frame_in_flight_fence));
  vkQueueSubmit(types::get(graphics_queue), 1, &submit_info,
                types::get(frame_in_flight_fence));
}

void
stage(types::device const device,
      types::physical_device const physical_device,
      types::queue const graphics_queue,
      types::command_pool const command_pool, types::buffer const buffer,
      utility::slice<std::byte const> src, types::device_size offset)
{
  auto const begin_info = []
  {
    auto info = VkCommandBufferBeginInfo();
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    info.pInheritanceInfo = nullptr;
    return info;
  }();

  auto const [staging_buffer, staging_buffer_memory] =
      detail::create_staging_buffer_and_memory(device, physical_device, src);

  auto requirements = VkMemoryRequirements();
  vkGetBufferMemoryRequirements(types::parent(staging_buffer),
                                types::get(staging_buffer), &requirements);

  auto const size = requirements.size;

  auto const copy_region = [size, offset]
  {
    auto region = VkBufferCopy();
    region.srcOffset = 0;
    region.dstOffset = offset;
    region.size = size;
    return region;
  }();

  auto const command_buffer =
      detail::create_staging_command_buffer(device, command_pool);

  vkBeginCommandBuffer(types::get(command_buffer), &begin_info);
  vkCmdCopyBuffer(types::get(command_buffer), types::get(staging_buffer),
                  types::get(buffer), 1, &copy_region);
  vkEndCommandBuffer(types::get(command_buffer));

  detail::submit_staging_command_buffer(graphics_queue,
                                        types::get(command_buffer));
}

[[nodiscard]] utility::slice<std::byte>
map_memory(types::device const device, types::device_memory const memory,
           types::device_size const size,
           types::device_size const offset) noexcept
{
  void * data = nullptr;
  vkMapMemory(types::get(device), types::get(memory), offset, size, 0, &data);
  return {utility::force_cast_to_byte(data), size};
}

void
transition_layout(types::device const device,
                  types::queue const graphics_queue,
                  types::command_pool const command_pool,
                  types::image const image, VkImageLayout const old_layout,
                  VkImageLayout const new_layout,
                  uint32_t const mipmap_levels) noexcept
{
  auto const [image_memory_barrier, source_stage, destination_stage] =
      [old_layout, new_layout, &image, mipmap_levels]
  {
    auto barrier = VkImageMemoryBarrier();
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = types::get(image);

    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else
    {
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipmap_levels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    auto source_stage = VkPipelineStageFlags();
    auto destination_stage = VkPipelineStageFlags();

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
             new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
      MVK_VERIFY_NOT_REACHED();
    }

    return std::make_tuple(barrier, source_stage, destination_stage);
  }();

  auto const begin_info = []
  {
    auto info = VkCommandBufferBeginInfo();
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    info.pInheritanceInfo = nullptr;
    return info;
  }();

  auto const command_buffer =
      detail::create_staging_command_buffer(device, command_pool);

  vkBeginCommandBuffer(types::get(command_buffer), &begin_info);
  vkCmdPipelineBarrier(types::get(command_buffer), source_stage,
                       destination_stage, 0, 0, nullptr, 0, nullptr, 1,
                       &image_memory_barrier);
  vkEndCommandBuffer(types::get(command_buffer));

  detail::submit_staging_command_buffer(graphics_queue,
                                        types::get(command_buffer));
}

void
stage(types::device const device,
      types::physical_device const physical_device,
      types::queue const graphics_queue, types::command_pool command_pool,
      types::image const buffer, utility::slice<std::byte const> src,
      uint32_t width, uint32_t height) noexcept
{
  auto const [staging_buffer, staging_buffer_memory] =
      detail::create_staging_buffer_and_memory(device, physical_device, src);

  auto const begin_info = []
  {
    auto info = VkCommandBufferBeginInfo();
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    info.pInheritanceInfo = nullptr;
    return info;
  }();

  auto const copy_region = [width, height]
  {
    auto region = VkBufferImageCopy();
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset.x = 0;
    region.imageOffset.y = 0;
    region.imageOffset.z = 0;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;
    return region;
  }();

  auto const command_buffer =
      detail::create_staging_command_buffer(device, command_pool);

  vkBeginCommandBuffer(types::get(command_buffer), &begin_info);
  vkCmdCopyBufferToImage(types::get(command_buffer),
                         types::get(staging_buffer), types::get(buffer),
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &copy_region);
  vkEndCommandBuffer(types::get(command_buffer));

  detail::submit_staging_command_buffer(graphics_queue,
                                        types::get(command_buffer));
}

void
generate_mipmaps(types::device const device,
                 types::queue const graphics_queue,
                 types::command_pool const command_pool,
                 types::image const image, uint32_t width, uint32_t height,
                 uint32_t mipmap_levels)
{
  if (mipmap_levels == 1 || mipmap_levels == 0)
  {
    return;
  }

  auto const begin_info = []
  {
    auto info = VkCommandBufferBeginInfo();
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    info.pInheritanceInfo = nullptr;
    return info;
  }();

  auto command_buffer =
      detail::create_staging_command_buffer(device, command_pool);
  vkBeginCommandBuffer(types::get(command_buffer), &begin_info);

  auto barrier = VkImageMemoryBarrier();
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = types::get(image);
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  auto mipmap_width = static_cast<int32_t>(width);
  auto mipmap_height = static_cast<int32_t>(height);

  auto const half = [](auto & num)
  {
    if (num > 1)
    {
      num /= 2;
      return num;
    }

    return 1;
  };

  for (auto i = uint32_t(0); i < (mipmap_levels - 1); ++i)
  {
    barrier.subresourceRange.baseMipLevel = i;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(types::get(command_buffer),
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);

    auto blit = VkImageBlit();
    blit.srcOffsets[0].x = 0;
    blit.srcOffsets[0].y = 0;
    blit.srcOffsets[0].z = 0;
    blit.srcOffsets[1].x = mipmap_width;
    blit.srcOffsets[1].y = mipmap_height;
    blit.srcOffsets[1].z = 1;
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = i;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0].x = 0;
    blit.dstOffsets[0].y = 0;
    blit.dstOffsets[0].z = 0;
    blit.dstOffsets[1].x = half(mipmap_width);
    blit.dstOffsets[1].y = half(mipmap_height);
    blit.dstOffsets[1].z = 1;
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = i + 1;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    vkCmdBlitImage(types::get(command_buffer), types::get(image),
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, types::get(image),
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                   VK_FILTER_LINEAR);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(types::get(command_buffer),
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &barrier);
  }

  barrier.subresourceRange.baseMipLevel = mipmap_levels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(types::get(command_buffer),
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                       0, nullptr, 1, &barrier);
  vkEndCommandBuffer(types::get(command_buffer));

  detail::submit_staging_command_buffer(graphics_queue,
                                        types::get(command_buffer));
}

[[nodiscard]] std::tuple<std::vector<unsigned char>, uint32_t, uint32_t>
load_texture(std::filesystem::path const & path)
{
  MVK_VERIFY(std::filesystem::exists(path));

  auto width = 0;
  auto height = 0;
  auto channels = 0;
  auto const pixels =
      stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

  auto buffer = std::vector<unsigned char>(static_cast<uint32_t>(width) *
                                           static_cast<uint32_t>(height) * 4 *
                                           sizeof(*pixels));
  std::copy(pixels,
            std::next(pixels, static_cast<int64_t>(std::size(buffer))),
            std::begin(buffer));

  stbi_image_free(pixels);

  return {std::move(buffer), width, height};
}

[[nodiscard]] std::pair<types::unique_buffer, types::unique_device_memory>
create_staging_buffer_and_memory(
    types::device const device, types::physical_device const physical_device,
    utility::slice<std::byte const> const src) noexcept
{
  auto const size = std::size(src);

  auto staging_buffer = [size, &device]
  {
    auto info = VkBufferCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    return types::unique_buffer::create(types::get(device), info);
  }();

  auto buffer_memory = [&device, physical_device, &staging_buffer]
  {
    auto requirements = VkMemoryRequirements();
    vkGetBufferMemoryRequirements(types::parent(staging_buffer),
                                  types::get(staging_buffer), &requirements);

    auto const type_bits = requirements.memoryTypeBits;
    auto const properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    auto const memory_type_index =
        find_memory_type(types::get(physical_device), type_bits, properties);

    MVK_VERIFY(memory_type_index.has_value());

    auto info = VkMemoryAllocateInfo();
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.allocationSize = requirements.size;
    info.memoryTypeIndex = memory_type_index.value();

    return types::unique_device_memory::create(types::get(device), info);
  }();

  vkBindBufferMemory(types::parent(staging_buffer),
                     types::get(staging_buffer), types::get(buffer_memory),
                     0);

  auto data = detail::map_memory(device, types::decay(buffer_memory), size);
  std::copy(std::begin(src), std::end(src), std::begin(data));
  vkUnmapMemory(types::parent(buffer_memory), types::get(buffer_memory));

  return {std::move(staging_buffer), std::move(buffer_memory)};
}

[[nodiscard]] types::unique_command_buffer
create_staging_command_buffer(types::device const device,
                              types::command_pool const pool) noexcept
{
  return std::move(create_command_buffers(
      device, pool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY)[0]);
}

void
submit_staging_command_buffer(
    types::queue const graphics_queue,
    types::command_buffer const command_buffer) noexcept
{
  auto submit_info = VkSubmitInfo();
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &types::get(command_buffer);

  vkQueueSubmit(types::get(graphics_queue), 1, &submit_info, nullptr);
  vkQueueWaitIdle(types::get(graphics_queue));
}

[[nodiscard]] std::optional<uint32_t>
find_memory_type(VkPhysicalDevice const physical_device,
                 uint32_t const filter,
                 VkMemoryPropertyFlags const properties_flags)
{
  auto memory_properties = VkPhysicalDeviceMemoryProperties();
  vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

  auto const type_count = memory_properties.memoryTypeCount;

  for (auto i = uint32_t(0); i < type_count; ++i)
  {
    auto const & current_type = memory_properties.memoryTypes[i];
    auto const current_flags = current_type.propertyFlags;
    auto const matches_flags = (current_flags & properties_flags) != 0U;
    auto const matches_filter = (filter & (1U << i)) != 0U;

    if (matches_flags && matches_filter)
    {
      return i;
    }
  }

  return std::nullopt;
}

[[nodiscard]] std::optional<uint32_t>
next_swapchain_image(VkDevice const device, VkSwapchainKHR const swapchain,
                     VkSemaphore const semaphore, VkFence const fence)
{
  auto index = uint32_t(0);

  auto const result = vkAcquireNextImageKHR(
      device, swapchain, std::numeric_limits<uint64_t>::max(), semaphore,
      fence, &index);

  if (result != VK_ERROR_OUT_OF_DATE_KHR)
  {
    return index;
  }

  return std::nullopt;
}

[[nodiscard]] uint32_t
calculate_mimap_levels(uint32_t const height, uint32_t const width) noexcept
{
  return static_cast<uint32_t>(
      std::floor(std::log2(std::max(height, width))) + 1);
}

} // namespace mvk::detail
