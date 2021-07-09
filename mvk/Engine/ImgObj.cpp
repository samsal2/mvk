#include "Engine/ImgObj.hpp"

#include "Detail/Misc.hpp"
#include "Engine/AllocatorContext.hpp"

namespace Mvk::Engine
{
  ImgObj::ImgObj( size_t Width, size_t Height, Allocator Alloc ) noexcept
    : MipLvl( Detail::calcMipLvl( Width, Height ) )
    , Width( Width )
    , Height( Height )
    , Stage( Width * Height * RGBASize, Alloc )
    , Img( VK_NULL_HANDLE )
    , ImgView( VK_NULL_HANDLE )
    , Sampler( VK_NULL_HANDLE )
  {
    auto ImgCrtInfo          = VkImageCreateInfo();
    ImgCrtInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImgCrtInfo.imageType     = VK_IMAGE_TYPE_2D;
    ImgCrtInfo.extent.width  = Width;
    ImgCrtInfo.extent.height = Height;
    ImgCrtInfo.extent.depth  = 1;
    ImgCrtInfo.mipLevels     = MipLvl;
    ImgCrtInfo.arrayLayers   = 1;
    ImgCrtInfo.format        = VK_FORMAT_R8G8B8A8_SRGB;
    ImgCrtInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ImgCrtInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImgCrtInfo.usage         = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ImgCrtInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ImgCrtInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    ImgCrtInfo.flags         = 0;

    auto const Device = VulkanContext::the().getDevice();

    auto Result = vkCreateImage( Device, &ImgCrtInfo, nullptr, &Img );

    auto Req = VkMemoryRequirements();
    vkGetImageMemoryRequirements( Device, Img, &Req );

    auto Allocation = Stage.getAllocator().allocate( AllocationType::GpuOnly, Req.size, Req.alignment, Req.memoryTypeBits );

    ID = Allocation.ID;
    vkBindImageMemory( Device, Img, Allocation.Mem, Allocation.Off );

    auto ImgViewCrtInfo                            = VkImageViewCreateInfo();
    ImgViewCrtInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImgViewCrtInfo.image                           = Img;
    ImgViewCrtInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    ImgViewCrtInfo.format                          = VK_FORMAT_R8G8B8A8_SRGB;
    ImgViewCrtInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImgViewCrtInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImgViewCrtInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImgViewCrtInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImgViewCrtInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    ImgViewCrtInfo.subresourceRange.baseMipLevel   = 0;
    ImgViewCrtInfo.subresourceRange.levelCount     = ImgCrtInfo.mipLevels;
    ImgViewCrtInfo.subresourceRange.baseArrayLayer = 0;
    ImgViewCrtInfo.subresourceRange.layerCount     = 1;

    Result = vkCreateImageView( Device, &ImgViewCrtInfo, nullptr, &ImgView );
    MVK_VERIFY( Result == VK_SUCCESS );

    auto SamplerCrtInfo                    = VkSamplerCreateInfo();
    SamplerCrtInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCrtInfo.magFilter               = VK_FILTER_LINEAR;
    SamplerCrtInfo.minFilter               = VK_FILTER_LINEAR;
    SamplerCrtInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerCrtInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerCrtInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerCrtInfo.anisotropyEnable        = VK_TRUE;
    SamplerCrtInfo.anisotropyEnable        = VK_TRUE;
    SamplerCrtInfo.maxAnisotropy           = 16;
    SamplerCrtInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    SamplerCrtInfo.unnormalizedCoordinates = VK_FALSE;
    SamplerCrtInfo.compareEnable           = VK_FALSE;
    SamplerCrtInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    SamplerCrtInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerCrtInfo.mipLodBias              = 0.0F;
    SamplerCrtInfo.minLod                  = 0.0F;
    SamplerCrtInfo.maxLod                  = std::numeric_limits<float>::max();

    Result = vkCreateSampler( Device, &SamplerCrtInfo, nullptr, &Sampler );
    MVK_VERIFY( Result == VK_SUCCESS );
  }

  void ImgObj::map( VkCommandBuffer CmdBuff, std::span<std::byte const> Data ) noexcept
  {
    Stage.map( Data ).copyTo( CmdBuff, Img, Width, Height );
  }

  void ImgObj::transitionLayout( VkCommandBuffer CmdBuff, VkImageLayout OldLay, VkImageLayout NewLay ) noexcept
  {
    Detail::transitionImgLayout( CmdBuff, Img, OldLay, NewLay, MipLvl );
  }

  void ImgObj::generateMips( VkCommandBuffer CmdBuff ) noexcept
  {
    Detail::generateMip( CmdBuff, Img, Width, Height, MipLvl );
  }

  ImgObj::~ImgObj() noexcept
  {
    auto const Device = VulkanContext::the().getDevice();
    vkDestroyImage( Device, Img, nullptr );
    vkDestroyImageView( Device, ImgView, nullptr );
    vkDestroySampler( Device, Sampler, nullptr );

    Stage.getAllocator().free( ID );
  }

}  // namespace Mvk::Engine