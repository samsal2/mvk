#include "vk_types/debug_messenger.hpp"

#include "vk_types/instance.hpp"

namespace mvk::vk_types
{

debug_messenger::debug_messenger(VkInstance const instance)
  : wrapper(
      validation::setup_debug_messenger(instance),
      make_deleter(instance))
{
}

} // namespace mvk::vk_types
