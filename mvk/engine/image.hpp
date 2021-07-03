#ifndef MVK_ENGINE_IMAGE_HPP_INCLUDED
#define MVK_ENGINE_IMAGE_HPP_INCLUDED

#include "engine/allocate.hpp"
#include "engine/context.hpp"

namespace mvk::engine
{
  void transition_layout( context const & ctx,
                          types::image    image,
                          VkImageLayout   old_layout,
                          VkImageLayout   new_layout,
                          uint32_t        mipmap_levels ) noexcept;

  void generate_mipmaps(
    context const & ctx, types::image image, uint32_t width, uint32_t height, uint32_t mipmap_levels ) noexcept;

  void stage_image(
    context & ctx, staging_allocation allocation, uint32_t width, uint32_t height, types::image image ) noexcept;

}  // namespace mvk::engine

#endif
