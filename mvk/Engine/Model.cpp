#include "Model.hpp"

#include "Detail/Misc.hpp"

namespace Mvk::Engine
{
  Model::Model( VkDeviceSize VtxSize, VkDeviceSize IdxSize, VkDeviceSize PvmSize, size_t TexWidth, size_t TexHeight ) noexcept
    : Ubo( PvmSize ), Vbo( VtxSize ), Ibo( IdxSize ), Tex( TexWidth, TexHeight )
  {}

}  // namespace Mvk::Engine
