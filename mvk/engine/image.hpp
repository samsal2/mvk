#ifndef MVK_ENGINE_IMAGE_HPP_INCLUDED
#define MVK_ENGINE_IMAGE_HPP_INCLUDED

#include "engine/allocate.hpp"
#include "engine/context.hpp"

namespace mvk::engine
{
  void
    trainstionLay(In<Context> Ctx, VkImage Img, VkImageLayout OldLay, VkImageLayout NewLay, uint32_t MipLvl) noexcept;

  void generateMip(In<Context> Ctx, VkImage Img, uint32_t Width, uint32_t Height, uint32_t MipLvl) noexcept;
  void stageImage(In<Context> Ctx, StagingAlloc Alloc, uint32_t Width, uint32_t Height, VkImage Image) noexcept;

}  // namespace mvk::engine

#endif
