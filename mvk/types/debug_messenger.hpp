#ifndef MVK_TYPES_DEBUG_MESSENGER_HPP_INCLUDED
#define MVK_TYPES_DEBUG_MESSENGER_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"
#include "types/validation/validation.hpp"

namespace mvk::types
{

class instance;

class debug_messenger
    : public detail::wrapper<
          detail::deleter<validation::destroy_debug_messenger>,
          detail::handle<VkDebugUtilsMessengerEXT>,
          detail::parent<VkInstance>>
{
public:
  constexpr debug_messenger() noexcept = default;
  explicit debug_messenger(VkInstance instance) noexcept;
};

} // namespace mvk::types

#endif
