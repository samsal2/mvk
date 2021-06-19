#ifndef MVK_TYPES_COMMON_HPP_INCLUDED
#define MVK_TYPES_COMMON_HPP_INCLUDED

#include "utility/verify.hpp"

#include <string>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <vulkan/vulkan.h>

namespace mvk
{

namespace types
{

using device_size = VkDeviceSize;

}
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

namespace types
{

template <typename Wrapper> constexpr decltype(auto) get(Wrapper const & wrapper) noexcept
{
        return wrapper.get();
}

template <typename Wrapper> constexpr decltype(auto) parent(Wrapper const & wrapper) noexcept
{
        return wrapper.parent();
}

template <typename Wrapper> constexpr decltype(auto) pool(Wrapper const & wrapper) noexcept
{
        return wrapper.pool();
}

} // namespace types

} // namespace mvk

#endif
