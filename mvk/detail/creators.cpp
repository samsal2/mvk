#include "detail/creators.hpp"

namespace mvk::detail
{

[[nodiscard]] vk_types::shader_module
create_shader_module(
  vk_types::device const &   device,
  utility::slice<char> const code)
{
  auto info     = VkShaderModuleCreateInfo();
  info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.codeSize = static_cast<uint32_t>(std::size(code));
  info.pCode    = reinterpret_cast<uint32_t const *>(std::data(code));
  return vk_types::shader_module(device.get(), info);
}

} // namespace mvk::detail
