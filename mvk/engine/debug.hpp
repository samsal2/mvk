#ifndef MVK_ENGINE_DEBUG_HPP_INCLUDED
#define MVK_ENGINE_DEBUG_HPP_INCLUDED

#include <iostream>
#include <vulkan/vulkan.h>

namespace mvk::engine
{
  static VKAPI_ATTR VKAPI_CALL VkBool32
    debug_callback( [[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT       severity,
                    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT              type,
                    [[maybe_unused]] VkDebugUtilsMessengerCallbackDataEXT const * data,
                    [[maybe_unused]] void *                                       p_user_data )
  {
    std::cerr << data->pMessage << '\n';
    return VK_FALSE;
  }

  static constexpr auto debug_create_info = []
  {
    auto tmp            = VkDebugUtilsMessengerCreateInfoEXT();
    tmp.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    tmp.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |  // NOLINT
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    tmp.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |     // NOLINT
                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |  // NO
                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    tmp.pfnUserCallback = debug_callback;
    tmp.pUserData       = nullptr;
    return tmp;
  }();

}  // namespace mvk::engine

#endif
