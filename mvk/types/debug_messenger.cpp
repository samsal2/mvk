#include "types/debug_messenger.hpp"

#include "types/instance.hpp"

namespace mvk::types
{

debug_messenger::debug_messenger(VkInstance const instance) : wrapper(validation::setup_debug_messenger(instance), instance)
{
}

} // namespace mvk::types
