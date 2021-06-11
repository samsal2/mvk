#include "vk_types/debug_messenger.hpp"

#include "vk_types/instance.hpp"

namespace mvk::vk_types
{

debug_messenger::debug_messenger(VkInstance const instance) : unique_wrapper_with_parent(validation::setup_debug_messenger(instance), instance)
{
}

} // namespace mvk::vk_types
