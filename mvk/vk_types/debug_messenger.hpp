#ifndef MVK_VK_TYPES_DEBUG_MESSENGER_HPP_INCLUDED
#define MVK_VK_TYPES_DEBUG_MESSENGER_HPP_INCLUDED

#include "vk_types/common.hpp"
#include "vk_types/detail/wrapper.hpp"
#include "vk_types/validation/validation.hpp"

namespace mvk::vk_types
{

class instance;

class debug_messenger
  : public detail::
      wrapper<VkDebugUtilsMessengerEXT, validation::destroy_debug_messenger>
{
public:
  constexpr debug_messenger() noexcept = default;
  explicit debug_messenger(VkInstance instance);
};

} // namespace mvk::vk_types

#endif // MVK_VK_TYPES_DEBUG_MESSENGER_HPP_INCLUDED