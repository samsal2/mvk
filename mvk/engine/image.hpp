#ifndef MVK_ENGINE_IMAGE_HPP_INCLUDED
#define MVK_ENGINE_IMAGE_HPP_INCLUDED

#include "engine/Context.hpp"
#include "engine/allocate.hpp"

namespace mvk::engine
{
  void transition_layout(
    Context const & Ctx, VkImage Img, VkImageLayout OldLay, VkImageLayout NewLay, uint32_t MipLvl ) noexcept;

  void generate_mipmaps( Context const & Ctx, VkImage Img, uint32_t Width, uint32_t Height, uint32_t MipLvl ) noexcept;
  void stage_image( Context const & Ctx, StagingAlloc Alloc, uint32_t Width, uint32_t Height, VkImage Image ) noexcept;

}  // namespace mvk::engine

#endif
