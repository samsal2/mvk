#pragma once

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace Mvk
{
  struct vertex
  {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texture_coord;
  };

  struct PVM
  {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
  };
}  // namespace Mvk
