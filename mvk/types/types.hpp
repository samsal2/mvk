#ifndef MVK_TYPES_TYPES_HPP_INCLUDED
#define MVK_TYPES_TYPES_HPP_INCLUDED

#include "types/window.hpp"
#include "validation/validation.hpp"
#include "wrapper/any_wrapper.hpp"

#include <vulkan/vulkan.h>

namespace mvk::types
{
using device_size = VkDeviceSize;
using queue_index = uint32_t;

template <typename Wrapper>
constexpr decltype(auto)
get(Wrapper const & wrapper) noexcept
{
  return wrapper.get();
}

template <typename Wrapper>
constexpr decltype(auto)
get(Wrapper & wrapper) noexcept
{
  return wrapper.get();
}

template <typename Wrapper>
constexpr decltype(auto)
parent(Wrapper const & wrapper) noexcept
{
  return wrapper.deleter().parent();
}

template <typename Wrapper>
constexpr decltype(auto)
pool(Wrapper const & wrapper) noexcept
{
  return wrapper.deleter().parent();
}

template <typename... Args>
constexpr auto
decay(wrapper::any_wrapper<Args...> const & wrapper) noexcept
{
  using handle =
      decltype(wrapper::select<wrapper::options::handle>(Args{}...));

  return wrapper::any_wrapper<
      wrapper::options::storage<wrapper::storage::handle_only>,
      wrapper::options::handle<handle>>(get(wrapper));
}

} // namespace mvk::types

namespace mvk::types
{
using unique_buffer = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::object_create>,
    wrapper::options::creator_call<vkCreateBuffer>,
    wrapper::options::handle<VkBuffer>, wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyBuffer>>;

using buffer = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkBuffer>>;

using unique_pipeline_layout = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::object_create>,
    wrapper::options::creator_call<vkCreatePipelineLayout>,
    wrapper::options::handle<VkPipelineLayout>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyPipelineLayout>>;

using pipeline_layout = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkPipelineLayout>>;

using unique_command_buffer = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_free>,
    wrapper::options::creator<wrapper::creator::object_allocate>,
    wrapper::options::creator_call<vkAllocateCommandBuffers>,
    wrapper::options::handle<VkCommandBuffer>,
    wrapper::options::parent<VkDevice>, wrapper::options::pool<VkCommandPool>,
    wrapper::options::deleter_call<vkFreeCommandBuffers>>;

using command_buffer = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkCommandBuffer>>;

using unique_command_pool = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::object_create>,
    wrapper::options::creator_call<vkCreateCommandPool>,
    wrapper::options::handle<VkCommandPool>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyCommandPool>>;

using command_pool = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkCommandPool>>;

using unique_debug_messenger = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::object_create>,
    wrapper::options::creator_call<validation::setup_debug_messenger>,
    wrapper::options::handle<VkDebugUtilsMessengerEXT>,
    wrapper::options::parent<VkInstance>,
    wrapper::options::deleter_call<validation::destroy_debug_messenger>>;

using unique_descriptor_pool = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::object_create>,
    wrapper::options::creator_call<vkCreateDescriptorPool>,
    wrapper::options::handle<VkDescriptorPool>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyDescriptorPool>>;

using descriptor_pool = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkDescriptorPool>>;

using unique_descriptor_set_layout = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::object_create>,
    wrapper::options::creator_call<vkCreateDescriptorSetLayout>,
    wrapper::options::handle<VkDescriptorSetLayout>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyDescriptorSetLayout>>;

using descriptor_set_layout = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkDescriptorSetLayout>>;

using unique_descriptor_set = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_free>,
    wrapper::options::creator<wrapper::creator::object_allocate>,
    wrapper::options::creator_call<vkAllocateDescriptorSets>,
    wrapper::options::handle<VkDescriptorSet>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::pool<VkDescriptorPool>,
    wrapper::options::deleter_call<vkFreeDescriptorSets>>;

using descriptor_set = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkDescriptorSet>>;

using unique_device_memory = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::object_create>,
    wrapper::options::creator_call<vkAllocateMemory>,
    wrapper::options::handle<VkDeviceMemory>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkFreeMemory>>;

using device_memory = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkDeviceMemory>>;

using queue = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::creator<wrapper::creator::handle_get>,
    wrapper::options::creator_call<vkGetDeviceQueue>,
    wrapper::options::handle<VkQueue>>;

using physical_device = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkPhysicalDevice>>;

using unique_device = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::owner_destroy>,
    wrapper::options::creator<wrapper::creator::owner_create>,
    wrapper::options::creator_call<vkCreateDevice>,
    wrapper::options::handle<VkDevice>,
    wrapper::options::deleter_call<vkDestroyDevice>>;

using device = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkDevice>>;

using unique_fence = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::object_create>,
    wrapper::options::creator_call<vkCreateFence>,
    wrapper::options::handle<VkFence>, wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyFence>>;

using fence = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkFence>>;

using unique_framebuffer = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::object_create>,
    wrapper::options::creator_call<vkCreateFramebuffer>,
    wrapper::options::handle<VkFramebuffer>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyFramebuffer>>;

using framebuffer = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkFramebuffer>>;

using unique_image_view = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::object_create>,
    wrapper::options::creator_call<vkCreateImageView>,
    wrapper::options::handle<VkImageView>, wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyImageView>>;

using image_view = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkImageView>>;

using unique_instance = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::owner_destroy>,
    wrapper::options::creator<wrapper::creator::owner_create>,
    wrapper::options::creator_call<vkCreateInstance>,
    wrapper::options::handle<VkInstance>,
    wrapper::options::deleter_call<vkDestroyInstance>>;

using instance = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkInstance>>;

using unique_pipeline = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::object_create>,
    wrapper::options::creator_call<vkCreateGraphicsPipelines>,
    wrapper::options::handle<VkPipeline>, wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyPipeline>>;

using pipeline = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkPipeline>>;

using unique_render_pass = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::object_create>,
    wrapper::options::creator_call<vkCreateRenderPass>,
    wrapper::options::handle<VkRenderPass>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyRenderPass>>;

using render_pass = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkRenderPass>>;

using unique_sampler = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::object_create>,
    wrapper::options::creator_call<vkCreateSampler>,
    wrapper::options::handle<VkSampler>, wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroySampler>>;

using sampler = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkSampler>>;

using unique_semaphore = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::object_create>,
    wrapper::options::creator_call<vkCreateSemaphore>,
    wrapper::options::handle<VkSemaphore>, wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroySemaphore>>;

using semaphore = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkSemaphore>>;

using unique_surface = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::handle<VkSurfaceKHR>,
    wrapper::options::parent<VkInstance>,
    wrapper::options::deleter_call<vkDestroySurfaceKHR>>;

using surface = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkSurfaceKHR>>;

using unique_image = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::object_create>,
    wrapper::options::creator_call<vkCreateImage>,
    wrapper::options::handle<VkImage>, wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyImage>>;

using image = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkImage>>;

using unique_shader_module = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::object_create>,
    wrapper::options::creator_call<vkCreateShaderModule>,
    wrapper::options::handle<VkShaderModule>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyShaderModule>>;

using shader_module = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkShaderModule>>;

using unique_swapchain = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::object_create>,
    wrapper::options::creator_call<vkCreateSwapchainKHR>,
    wrapper::options::handle<VkSwapchainKHR>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroySwapchainKHR>>;

using swapchain = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkSwapchainKHR>>;

} // namespace mvk::types

#endif
