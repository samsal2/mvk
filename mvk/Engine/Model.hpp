#pragma once

#include "Engine/IdxBuffObj.hpp"
#include "Engine/ImgObj.hpp"
#include "Engine/UniformBuffObj.hpp"
#include "Engine/VtxBuffObj.hpp"
#include "Utility/Badge.hpp"
#include "Utility/Macros.hpp"

namespace Mvk::Engine
{
  class VulkanRenderer;

  struct Model
  {
  public:
    // All sizes in bytes
    // Take ownership of the DescSet
    Model( VkDeviceSize VtxSize, VkDeviceSize IdxSize, VkDeviceSize PvmSize, size_t TexWidth, size_t TexHeight ) noexcept;

    UniformBuffObj Ubo;
    VtxBuffObj     Vbo;
    IdxBuffObj     Ibo;
    ImgObj         Tex;
  };
}  // namespace Mvk::Engine