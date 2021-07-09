#include "Detail/Misc.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma clang diagnostic pop

#include <array>
#include <cmath>
#include <optional>
#include <utility>
#include <vector>

namespace Mvk::Detail
{
  [[nodiscard]] std::tuple<std::vector<unsigned char>, uint32_t, uint32_t> loadTex( std::filesystem::path const & Path )
  {
    MVK_VERIFY( std::filesystem::exists( Path ) );

    auto       Width    = 0;
    auto       Height   = 0;
    auto       Channels = 0;
    auto const Pixels   = stbi_load( Path.c_str(), &Width, &Height, &Channels, STBI_rgb_alpha );

    auto Buff = std::vector<unsigned char>( static_cast<uint32_t>( Width ) * static_cast<uint32_t>( Height ) * 4 * sizeof( *Pixels ) );
    std::copy( Pixels, std::next( Pixels, static_cast<int64_t>( std::size( Buff ) ) ), std::begin( Buff ) );

    stbi_image_free( Pixels );

    return { std::move( Buff ), Width, Height };
  }

  [[nodiscard]] std::optional<uint32_t> queryMemType( VkPhysicalDevice PhysicalDevice, uint32_t Filter, VkMemoryPropertyFlags PropFlags )
  {
    auto MemProp = VkPhysicalDeviceMemoryProperties();
    vkGetPhysicalDeviceMemoryProperties( PhysicalDevice, &MemProp );

    auto const TypeCount = MemProp.memoryTypeCount;

    for ( auto i = uint32_t( 0 ); i < TypeCount; ++i )
    {
      auto const & CurrentType   = MemProp.memoryTypes[i];
      auto const   CurrentFlags  = CurrentType.propertyFlags;
      auto const   MatchesFlags  = ( CurrentFlags & PropFlags ) != 0U;
      auto const   MatchesFilter = ( Filter & ( 1U << i ) ) != 0U;

      if ( MatchesFlags && MatchesFilter )
      {
        return i;
      }
    }

    return std::nullopt;
  }

  [[nodiscard]] std::optional<uint32_t>
    querySwapchainImg( VkDevice const Device, VkSwapchainKHR const Swapchain, VkSemaphore const Semaphore, VkFence const Fence )
  {
    auto Idx = uint32_t( 0 );

    auto const Result = vkAcquireNextImageKHR( Device, Swapchain, std::numeric_limits<uint64_t>::max(), Semaphore, Fence, &Idx );

    if ( Result != VK_ERROR_OUT_OF_DATE_KHR )
    {
      return Idx;
    }

    return std::nullopt;
  }

  [[nodiscard]] uint32_t calcMipLvl( uint32_t const Height, uint32_t const Width ) noexcept
  {
    return static_cast<uint32_t>( std::floor( std::log2( std::max( Height, Width ) ) ) + 1 );
  }

  void transitionImgLayout( VkCommandBuffer CmdBuff, VkImage Img, VkImageLayout OldLay, VkImageLayout NewLay, uint32_t MipLvl ) noexcept
  {
    auto ImgMemBarrier                = VkImageMemoryBarrier();
    ImgMemBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    ImgMemBarrier.oldLayout           = OldLay;
    ImgMemBarrier.newLayout           = NewLay;
    ImgMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ImgMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ImgMemBarrier.image               = Img;

    if ( NewLay == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
    {
      ImgMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else
    {
      ImgMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    ImgMemBarrier.subresourceRange.baseMipLevel   = 0;
    ImgMemBarrier.subresourceRange.levelCount     = MipLvl;
    ImgMemBarrier.subresourceRange.baseArrayLayer = 0;
    ImgMemBarrier.subresourceRange.layerCount     = 1;

    auto SrcStage = VkPipelineStageFlags();
    auto DstStage = VkPipelineStageFlags();

    if ( OldLay == VK_IMAGE_LAYOUT_UNDEFINED && NewLay == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
    {
      ImgMemBarrier.srcAccessMask = 0;
      ImgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      DstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if ( OldLay == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLay == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
    {
      ImgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      ImgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      DstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if ( OldLay == VK_IMAGE_LAYOUT_UNDEFINED && NewLay == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
    {
      ImgMemBarrier.srcAccessMask = 0;
      ImgMemBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      SrcStage                    = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      DstStage                    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
      MVK_VERIFY_NOT_REACHED();
    }

    vkCmdPipelineBarrier( CmdBuff, SrcStage, DstStage, 0, 0, nullptr, 0, nullptr, 1, &ImgMemBarrier );
  }

  void generateMip( VkCommandBuffer CmdBuff, VkImage Img, size_t Width, size_t Height, uint32_t MipLvl ) noexcept
  {
    if ( MipLvl == 1 || MipLvl == 0 )
    {
      return;
    }

    auto ImgMemBarrier                            = VkImageMemoryBarrier();
    ImgMemBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    ImgMemBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    ImgMemBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    ImgMemBarrier.image                           = Img;
    ImgMemBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    ImgMemBarrier.subresourceRange.baseArrayLayer = 0;
    ImgMemBarrier.subresourceRange.layerCount     = 1;
    ImgMemBarrier.subresourceRange.levelCount     = 1;

    auto MipWidth  = static_cast<int32_t>( Width );
    auto MipHeight = static_cast<int32_t>( Height );

    auto const half = []( auto & num )
    {
      if ( num > 1 )
      {
        num /= 2;
        return num;
      }

      return 1;
    };

    for ( auto i = uint32_t( 0 ); i < ( MipLvl - 1 ); ++i )
    {
      ImgMemBarrier.subresourceRange.baseMipLevel = i;
      ImgMemBarrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      ImgMemBarrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      ImgMemBarrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
      ImgMemBarrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

      vkCmdPipelineBarrier(
        CmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &ImgMemBarrier );

      auto ImgBlit                          = VkImageBlit();
      ImgBlit.srcOffsets[0].x               = 0;
      ImgBlit.srcOffsets[0].y               = 0;
      ImgBlit.srcOffsets[0].z               = 0;
      ImgBlit.srcOffsets[1].x               = MipWidth;
      ImgBlit.srcOffsets[1].y               = MipHeight;
      ImgBlit.srcOffsets[1].z               = 1;
      ImgBlit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      ImgBlit.srcSubresource.mipLevel       = i;
      ImgBlit.srcSubresource.baseArrayLayer = 0;
      ImgBlit.srcSubresource.layerCount     = 1;
      ImgBlit.dstOffsets[0].x               = 0;
      ImgBlit.dstOffsets[0].y               = 0;
      ImgBlit.dstOffsets[0].z               = 0;
      ImgBlit.dstOffsets[1].x               = half( MipWidth );
      ImgBlit.dstOffsets[1].y               = half( MipHeight );
      ImgBlit.dstOffsets[1].z               = 1;
      ImgBlit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      ImgBlit.dstSubresource.mipLevel       = i + 1;
      ImgBlit.dstSubresource.baseArrayLayer = 0;
      ImgBlit.dstSubresource.layerCount     = 1;

      vkCmdBlitImage(
        CmdBuff, Img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImgBlit, VK_FILTER_LINEAR );

      ImgMemBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      ImgMemBarrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      ImgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      ImgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      vkCmdPipelineBarrier(
        CmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &ImgMemBarrier );
    }

    ImgMemBarrier.subresourceRange.baseMipLevel = MipLvl - 1;
    ImgMemBarrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    ImgMemBarrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ImgMemBarrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
    ImgMemBarrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
      CmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &ImgMemBarrier );
  }

}  // namespace Mvk::Detail
