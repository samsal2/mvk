#include "types/shader_module.hpp"

namespace mvk::types
{

shader_module::shader_module(VkDevice const device, VkShaderModuleCreateInfo const & create_info) : wrapper(nullptr, device)
{
    [[maybe_unused]] auto const result = vkCreateShaderModule(parent(), &create_info, nullptr, &reference());

    MVK_VERIFY(VK_SUCCESS == result);
}

shader_stage_builder &
shader_stage_builder::add_stage(shader_module shader_module, VkShaderStageFlagBits const stage)
{
    auto info   = VkPipelineShaderStageCreateInfo();
    info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage  = stage;
    info.module = shader_module.get();
    info.pName  = "main";

    shader_modules_.push_back(std::move(shader_module));
    stages_.push_back(info);

    return *this;
}

} // namespace mvk::types
