#ifndef MVK_VK_TYPES_SHADER_MODULE_HPP_INCLUDED
#define MVK_VK_TYPES_SHADER_MODULE_HPP_INCLUDED

#include "utility/slice.hpp"
#include "vk_types/common.hpp"
#include "vk_types/detail/wrapper.hpp"

namespace mvk::vk_types
{

class shader_module : public detail::wrapper<VkShaderModule, vkDestroyShaderModule>
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
} // namespace mvk::vk_types

#endif
