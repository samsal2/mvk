#ifndef MVK_SHADER_TYPES_HPP_INCLUDED
#define MVK_SHADER_TYPES_HPP_INCLUDED

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace mvk
{
struct vertex
{
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 texture_coord;
};

struct pvm
{
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

} // namespace mvk

#endif
