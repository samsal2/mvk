#ifndef MVK_TYPES_VALIDATION_HPP_INCLUDE
#define MVK_TYPES_VALIDATION_HPP_INCLUDE

#include "utility/slice.hpp"

#include <array>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

namespace mvk::validation
{
#ifndef NDEBUG
  static constexpr auto g_use_validation = true;
#else
  static constexpr auto g_use_validation = false;
#endif

  static constexpr auto g_validation_layers = std::array{ "VK_LAYER_KHRONOS_validation" };

  static constexpr auto g_validation_instance_extensions = std::array{ VK_EXT_DEBUG_UTILS_EXTENSION_NAME };

  static VKAPI_ATTR VKAPI_CALL VkBool32
    debug_callback( [[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT       severity,
                    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT              type,
                    [[maybe_unused]] VkDebugUtilsMessengerCallbackDataEXT const * data,
                    [[maybe_unused]] void *                                       p_user_data )
  {
    std::cerr << data->pMessage << '\n';
    return VK_FALSE;
  }

  static constexpr auto g_debug_create_info = []
  {
    auto tmp            = VkDebugUtilsMessengerCreateInfoEXT();
    tmp.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    tmp.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    tmp.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    tmp.pfnUserCallback = debug_callback;
    tmp.pUserData       = nullptr;
    return tmp;
  }();

  [[nodiscard]] static bool is_layer_present( VkLayerProperties const                  layer,
                                              utility::slice<char const * const> const layers ) noexcept
  {
    auto const matches = [&layer]( auto const & current_layer )
    {
      auto const layer_name_data = std::data( layer.layerName );
      return std::strcmp( current_layer, layer_name_data ) == 0;
    };

    return std::any_of( std::begin( layers ), std::end( layers ), matches );
  }

  [[nodiscard]] static bool check_support() noexcept
  {
    if constexpr ( !g_use_validation )
    {
      return true;
    }

    auto layer_properties_count = uint32_t( 0 );
    vkEnumerateInstanceLayerProperties( &layer_properties_count, nullptr );

    auto layer_properties = std::vector<VkLayerProperties>( layer_properties_count );

    vkEnumerateInstanceLayerProperties( &layer_properties_count, std::data( layer_properties ) );

    auto const exists = []( auto const & available_layer )
    {
      return is_layer_present( available_layer, g_validation_layers );
    };

    return std::any_of( std::begin( layer_properties ), std::end( layer_properties ), exists );
  }

  [[nodiscard]] static VkDebugUtilsMessengerEXT setup_debug_messenger( VkInstance const instance )
  {
    if constexpr ( !g_use_validation )
    {
      return nullptr;
    }

    auto const f = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" ) );

    MVK_VERIFY( f );

    auto callback = VkDebugUtilsMessengerEXT();
    f( instance, &g_debug_create_info, nullptr, &callback );

    return callback;
  }

  static void destroy_debug_messenger( VkInstance const               instance,
                                       VkDebugUtilsMessengerEXT const messenger,
                                       VkAllocationCallbacks const *  callback )
  {
    if constexpr ( !g_use_validation )
    {
      return;
    }

    auto const f = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" ) );
    MVK_VERIFY( f );
    f( instance, messenger, callback );
  }

  [[nodiscard]] static constexpr utility::slice<char const * const> validation_layers_data() noexcept
  {
    if constexpr ( !g_use_validation )
    {
      return {};
    }

    return { g_validation_layers };
  }

  [[nodiscard]] static constexpr VkDebugUtilsMessengerCreateInfoEXT const * debug_create_info_ref() noexcept
  {
    if constexpr ( !g_use_validation )
    {
      return nullptr;
    }

    return &g_debug_create_info;
  }

  [[nodiscard]] static utility::slice<char const * const> required_instance_extensions()
  {
    if constexpr ( !g_use_validation )
    {
      return {};
    }

    return { g_validation_instance_extensions };
  }

}  // namespace mvk::validation

#endif
