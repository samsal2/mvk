#ifndef MVK_VK_TYPES_COMMAND_BUFFERS_HPP_INCLUDED
#define MVK_VK_TYPES_COMMAND_BUFFERS_HPP_INCLUDED

#include "vk_types/common.hpp"
#include "vk_types/detail/wrapper.hpp"
#include "vk_types/single_command_buffer.hpp"

namespace mvk::vk_types
{

class command_buffers
    : public detail::wrapper<detail::deleter<vkFreeCommandBuffers>, detail::handle<VkCommandBuffer>, detail::parent<VkDevice>, detail::pool<VkCommandPool>>
{
public:
    constexpr command_buffers() noexcept = default;

    command_buffers(VkDevice device, VkCommandBufferAllocateInfo const & allocate_info);

    [[nodiscard]] single_command_buffer
    begin(size_t index, VkCommandBufferBeginInfo const & begin_info) const noexcept;
};

} // namespace mvk::vk_types

#endif
