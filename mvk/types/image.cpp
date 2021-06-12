#include "types/image.hpp"

#include "types/command_buffers.hpp"
#include "types/command_pool.hpp"
#include "types/detail/misc.hpp"
#include "types/detail/staging.hpp"
#include "types/device.hpp"
#include "types/device_memory.hpp"
#include "utility/verify.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma clang diagnostic pop

namespace mvk::types
{

image::image(VkDevice const device, VkImageCreateInfo const & info)
    : wrapper(nullptr, device), mipmap_levels_(info.mipLevels)
{
  [[maybe_unused]] auto const result =
      vkCreateImage(parent(), &info, nullptr, &reference());
  MVK_VERIFY(VK_SUCCESS == result);
  vkGetImageMemoryRequirements(parent(), get(), &memory_requirements_);
}

image &
image::transition_layout(device const & device,
                         command_pool const & command_pool,
                         VkImageLayout const old_layout,
                         VkImageLayout const new_layout)
{
  auto const [image_memory_barrier, source_stage, destination_stage] =
      [old_layout, new_layout, this]
  {
    auto barrier = VkImageMemoryBarrier();
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = get();

    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else
    {
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipmap_levels();
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
      MVK_VERIFY(false);
    }

    return std::make_tuple(barrier, source_stage, destination_stage);
  }();

  auto command_buffer =
      detail::create_staging_command_buffer(device, command_pool);

  auto const begin_info = []
  {
    auto info = VkCommandBufferBeginInfo();
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    info.pInheritanceInfo = nullptr;
    return info;
  }();

  command_buffer.begin(0, begin_info)
      .pipeline_barrier({source_stage, destination_stage, 0}, {}, {},
                        {&image_memory_barrier, 1})
      .end();

  detail::submit_staging_command_buffer(device, command_buffer);
  return *this;
}

image &
image::stage(device const & device, command_pool const & command_pool,
             image::texture const & texture)
{
  auto const [staging_buffer, staging_buffer_memory] =
      detail::create_staging_buffer_and_memory(device,
                                               utility::as_bytes(texture));

  auto staging_command_buffer =
      detail::create_staging_command_buffer(device, command_pool);

  auto const begin_info = []
  {
    auto info = VkCommandBufferBeginInfo();
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    info.pInheritanceInfo = nullptr;
    return info;
  }();

  auto const copy_region = [&texture]
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
    region.imageExtent.width = texture.width();
    region.imageExtent.height = texture.height();
    region.imageExtent.depth = 1;
    return region;
  }();

  staging_command_buffer.begin(0, begin_info)
      .copy_buffer_to_image(
          {staging_buffer.get(), get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL},
          {&copy_region, 1})
      .end();

  detail::submit_staging_command_buffer(device, staging_command_buffer);
  return *this;
}

image &
image::generate_mipmaps(device const & device,
                        command_pool const & command_pool, uint32_t width,
                        uint32_t height)
{
  if (mipmap_levels() == 1 || mipmap_levels() == 0)
  {
    return *this;
  }

  auto staging_command_buffer =
      detail::create_staging_command_buffer(device, command_pool);

  auto barrier = VkImageMemoryBarrier();
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = get();
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  auto const begin_info = []
  {
    auto info = VkCommandBufferBeginInfo();
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    info.pInheritanceInfo = nullptr;
    return info;
  }();

  auto current_command_buffer = staging_command_buffer.begin(0, begin_info);

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

  for (auto i = uint32_t(0); i < (mipmap_levels() - 1); ++i)
  {
    barrier.subresourceRange.baseMipLevel = i;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    current_command_buffer.pipeline_barrier(
        {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0},
        {}, {}, {&barrier, 1});

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

    current_command_buffer.blit_image(
        {get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL},
        {get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL}, {&blit, 1},
        VK_FILTER_LINEAR);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    current_command_buffer.pipeline_barrier(
        {VK_PIPELINE_STAGE_TRANSFER_BIT,
         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0},
        {}, {}, {&barrier, 1});
  }

  barrier.subresourceRange.baseMipLevel = mipmap_levels() - 1;

  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  current_command_buffer.pipeline_barrier(
      {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
       0},
      {}, {}, {&barrier, 1});

  current_command_buffer.end();

  detail::submit_staging_command_buffer(device, staging_command_buffer);
  return *this;
}

image::texture::texture(std::filesystem::path const & path)
{
  MVK_VERIFY(std::filesystem::exists(path));

  auto width = 0;
  auto height = 0;
  auto channels = 0;

  auto const pixels =
      stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

  MVK_VERIFY(pixels);

  pixels_ = std::unique_ptr<value_type, deleter>(pixels);
  width_ = static_cast<uint32_t>(width);
  height_ = static_cast<uint32_t>(height);
  channels_ = static_cast<uint32_t>(channels);
  mipmap_levels_ = detail::calculate_mimap_levels(height_, width_);
}

void
image::texture::deleter::operator()(value_type * const pixels) const noexcept
{
  if (pixels != nullptr)
  {
    stbi_image_free(static_cast<stbi_uc *>(pixels));
  }
}

} // namespace mvk::types
