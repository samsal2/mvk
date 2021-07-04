#include "engine/image.hpp"

namespace mvk::engine
{
  void transition_layout( context const & ctx,
                          VkImage         image,
                          VkImageLayout   old_layout,
                          VkImageLayout   new_layout,
                          uint32_t        mipmap_levels ) noexcept
  {
    auto const [ image_memory_barrier, source_stage, destination_stage ] =
      [ old_layout, new_layout, &image, mipmap_levels ]
    {
      auto barrier                = VkImageMemoryBarrier();
      barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrier.oldLayout           = old_layout;
      barrier.newLayout           = new_layout;
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.image               = image;

      if ( new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
      {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      }
      else
      {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      }

      barrier.subresourceRange.baseMipLevel   = 0;
      barrier.subresourceRange.levelCount     = mipmap_levels;
      barrier.subresourceRange.baseArrayLayer = 0;
      barrier.subresourceRange.layerCount     = 1;

      auto source_stage      = VkPipelineStageFlags();
      auto destination_stage = VkPipelineStageFlags();

      if ( old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
      {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      }
      else if ( old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
      {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      }
      else if ( old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
                new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
      {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask =
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        source_stage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      }
      else
      {
        MVK_VERIFY_NOT_REACHED();
      }

      return std::make_tuple( barrier, source_stage, destination_stage );
    }();

    auto command_buffer_begin_info             = VkCommandBufferBeginInfo();
    command_buffer_begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;

    auto const command_buffer = allocate_single_use_command_buffer( ctx );

    vkBeginCommandBuffer( command_buffer, &command_buffer_begin_info );
    vkCmdPipelineBarrier(
      command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier );

    vkEndCommandBuffer( command_buffer );

    auto submit_info               = VkSubmitInfo();
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &command_buffer;

    vkQueueSubmit( ctx.graphics_queue, 1, &submit_info, nullptr );
    vkQueueWaitIdle( ctx.graphics_queue );
    vkFreeCommandBuffers( ctx.device, ctx.command_pool, 1, &command_buffer );
  }

  void generate_mipmaps(
    context const & ctx, VkImage image, uint32_t width, uint32_t height, uint32_t mipmap_levels ) noexcept
  {
    if ( mipmap_levels == 1 || mipmap_levels == 0 )
    {
      return;
    }

    auto command_buffer_begin_info             = VkCommandBufferBeginInfo();
    command_buffer_begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;

    auto command_buffer = allocate_single_use_command_buffer( ctx );
    vkBeginCommandBuffer( command_buffer, &command_buffer_begin_info );

    auto barrier                            = VkImageMemoryBarrier();
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.subresourceRange.levelCount     = 1;

    auto mipmap_width  = static_cast< int32_t >( width );
    auto mipmap_height = static_cast< int32_t >( height );

    auto const half = []( auto & num )
    {
      if ( num > 1 )
      {
        num /= 2;
        return num;
      }

      return 1;
    };

    for ( auto i = uint32_t( 0 ); i < ( mipmap_levels - 1 ); ++i )
    {
      barrier.subresourceRange.baseMipLevel = i;
      barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

      vkCmdPipelineBarrier( command_buffer,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0,
                            0,
                            nullptr,
                            0,
                            nullptr,
                            1,
                            &barrier );

      auto blit                          = VkImageBlit();
      blit.srcOffsets[ 0 ].x             = 0;
      blit.srcOffsets[ 0 ].y             = 0;
      blit.srcOffsets[ 0 ].z             = 0;
      blit.srcOffsets[ 1 ].x             = mipmap_width;
      blit.srcOffsets[ 1 ].y             = mipmap_height;
      blit.srcOffsets[ 1 ].z             = 1;
      blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.srcSubresource.mipLevel       = i;
      blit.srcSubresource.baseArrayLayer = 0;
      blit.srcSubresource.layerCount     = 1;
      blit.dstOffsets[ 0 ].x             = 0;
      blit.dstOffsets[ 0 ].y             = 0;
      blit.dstOffsets[ 0 ].z             = 0;
      blit.dstOffsets[ 1 ].x             = half( mipmap_width );
      blit.dstOffsets[ 1 ].y             = half( mipmap_height );
      blit.dstOffsets[ 1 ].z             = 1;
      blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.dstSubresource.mipLevel       = i + 1;
      blit.dstSubresource.baseArrayLayer = 0;
      blit.dstSubresource.layerCount     = 1;

      vkCmdBlitImage( command_buffer,
                      image,
                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      image,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      1,
                      &blit,
                      VK_FILTER_LINEAR );

      barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      vkCmdPipelineBarrier( command_buffer,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                            0,
                            0,
                            nullptr,
                            0,
                            nullptr,
                            1,
                            &barrier );
    }

    barrier.subresourceRange.baseMipLevel = mipmap_levels - 1;
    barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier( command_buffer,
                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                          0,
                          0,
                          nullptr,
                          0,
                          nullptr,
                          1,
                          &barrier );

    vkEndCommandBuffer( command_buffer );

    auto submit_info               = VkSubmitInfo();
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &command_buffer;

    vkQueueSubmit( ctx.graphics_queue, 1, &submit_info, nullptr );
    vkQueueWaitIdle( ctx.graphics_queue );
    vkFreeCommandBuffers( ctx.device, ctx.command_pool, 1, &command_buffer );
  }

  // buffers

  void stage_image(
    context const & ctx, staging_allocation allocation, uint32_t width, uint32_t height, VkImage image ) noexcept
  {
    auto command_buffer_begin_info             = VkCommandBufferBeginInfo();
    command_buffer_begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;

    auto copy_region                            = VkBufferImageCopy();
    copy_region.bufferOffset                    = allocation.offset_;
    copy_region.bufferRowLength                 = 0;
    copy_region.bufferImageHeight               = 0;
    copy_region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.imageSubresource.mipLevel       = 0;
    copy_region.imageSubresource.baseArrayLayer = 0;
    copy_region.imageSubresource.layerCount     = 1;
    copy_region.imageOffset.x                   = 0;
    copy_region.imageOffset.y                   = 0;
    copy_region.imageOffset.z                   = 0;
    copy_region.imageExtent.width               = width;
    copy_region.imageExtent.height              = height;
    copy_region.imageExtent.depth               = 1;

    auto const command_buffer = allocate_single_use_command_buffer( ctx );

    vkBeginCommandBuffer( command_buffer, &command_buffer_begin_info );

    vkCmdCopyBufferToImage(
      command_buffer, allocation.buffer_, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region );

    vkEndCommandBuffer( command_buffer );

    auto submit_info               = VkSubmitInfo();
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &command_buffer;

    vkQueueSubmit( ctx.graphics_queue, 1, &submit_info, nullptr );
    vkQueueWaitIdle( ctx.graphics_queue );
    vkFreeCommandBuffers( ctx.device, ctx.command_pool, 1, &command_buffer );
  }

}  // namespace mvk::engine
