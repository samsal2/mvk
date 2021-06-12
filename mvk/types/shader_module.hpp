#ifndef MVK_TYPES_SHADER_MODULE_HPP_INCLUDED
#define MVK_TYPES_SHADER_MODULE_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"
#include "utility/slice.hpp"

namespace mvk::types
{

class shader_module : public detail::wrapper<detail::deleter<vkDestroyShaderModule>, detail::handle<VkShaderModule>, detail::parent<VkDevice>>
{
public:
    constexpr shader_module() noexcept = default;

    shader_module(VkDevice device, VkShaderModuleCreateInfo const & create_info);
};

class shader_stage_builder
{
public:
    shader_stage_builder &
    add_stage(shader_module shader_module, VkShaderStageFlagBits stage);

    [[nodiscard]] constexpr utility::slice<VkPipelineShaderStageCreateInfo>
    stages() const noexcept;

private:
    std::vector<VkPipelineShaderStageCreateInfo> stages_;
    // Just to keep the shader modules alive for the duration of the stage
    // builder
    std::vector<shader_module> shader_modules_;
};

[[nodiscard]] constexpr utility::slice<VkPipelineShaderStageCreateInfo>
shader_stage_builder::stages() const noexcept
{
    return stages_;
}
} // namespace mvk::types

#endif
