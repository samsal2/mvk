#pragma once

#include "Engine/AllocatorContext.hpp"
#include "Engine/StagingBuffObj.hpp"
#include "Utility/Macros.hpp"

namespace Mvk::Engine
{
  class ImgObj
  {
  public:
    static constexpr auto RGBASize = 4;

    ImgObj( size_t Width, size_t Height, Allocator Alloc = Allocator() ) noexcept;
    MVK_DEFINE_NON_COPYABLE( ImgObj );
    MVK_DEFINE_NON_MOVABLE( ImgObj );
    ~ImgObj() noexcept;

    void map( VkCommandBuffer CmdBuff, std::span<std::byte const> Data ) noexcept;
    void transitionLayout( VkCommandBuffer CmdBuff, VkImageLayout OldLay, VkImageLayout NewLay ) noexcept;
    void generateMips( VkCommandBuffer CmdBuff ) noexcept;

    [[nodiscard]] constexpr VkImageView getImgView() noexcept
    {
      return ImgView;
    }

    [[nodiscard]] constexpr VkSampler getSampler() noexcept
    {
      return Sampler;
    }

  private:
    uint32_t       MipLvl;
    size_t         Width;
    size_t         Height;
    StagingBuffObj Stage;
    VkImage        Img;
    VkImageView    ImgView;
    VkSampler      Sampler;
    AllocationID   ID;
  };

}  // namespace Mvk::Engine
