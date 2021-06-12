#ifndef MVK_TYPES_COMMAND_BUFFERS_HPP_INCLUDED
#define MVK_TYPES_COMMAND_BUFFERS_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"
#include "types/single_command_buffer.hpp"

namespace mvk::types
{

class command_buffers
    : public detail::wrapper<detail::deleter<vkFreeCommandBuffers>,
                             detail::handle<std::vector<VkCommandBuffer>>,
                             detail::parent<VkDevice>,
                             detail::pool<VkCommandPool>>
{
public:
  constexpr command_buffers() noexcept = default;

  command_buffers(VkDevice device, VkCommandBufferAllocateInfo const & info);

  [[nodiscard]] single_command_buffer
  begin(size_t index,
        VkCommandBufferBeginInfo const & begin_info) const noexcept;
};

} // namespace mvk::types

#endif
