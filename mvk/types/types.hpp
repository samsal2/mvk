#ifndef MVK_TYPES_TMP_HPP_INCLUDED
#define MVK_TYPES_TMP_HPP_INCLUDED

#include "types/window.hpp"
#include "validation/validation.hpp"
#include "wrapper/handle_only.hpp"
#include "wrapper/wrapper.hpp"

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

template <typename Wrapper>
constexpr wrapper::decay_wrapper_t<Wrapper>
decay(Wrapper const & wrapper) noexcept
{
  return get(wrapper);
}

} // namespace mvk::types

namespace mvk::types
{
using unique_buffer = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::created>,
    wrapper::options::creator_call<vkCreateBuffer>,
    wrapper::options::handle<VkBuffer>, wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyBuffer>>;

using buffer = wrapper::decay_wrapper_t<unique_buffer>;

using unique_pipeline_layout = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::created>,
    wrapper::options::creator_call<vkCreatePipelineLayout>,
    wrapper::options::handle<VkPipelineLayout>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyPipelineLayout>>;

using pipeline_layout = wrapper::decay_wrapper_t<unique_pipeline_layout>;

using unique_command_buffer = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_free>,
    wrapper::options::creator<wrapper::creator::allocated>,
    wrapper::options::creator_call<vkAllocateCommandBuffers>,
    wrapper::options::handle<VkCommandBuffer>,
    wrapper::options::parent<VkDevice>, wrapper::options::pool<VkCommandPool>,
    wrapper::options::deleter_call<vkFreeCommandBuffers>>;

using command_buffer = wrapper::decay_wrapper_t<unique_command_buffer>;

using unique_command_pool = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::created>,
    wrapper::options::creator_call<vkCreateCommandPool>,
    wrapper::options::handle<VkCommandPool>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyCommandPool>>;

using command_pool = wrapper::decay_wrapper_t<unique_command_pool>;
using unique_debug_messenger = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::created>,
    wrapper::options::creator_call<validation::setup_debug_messenger>,
    wrapper::options::handle<VkDebugUtilsMessengerEXT>,
    wrapper::options::parent<VkInstance>,
    wrapper::options::deleter_call<validation::destroy_debug_messenger>>;

using unique_descriptor_pool = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::created>,
    wrapper::options::creator_call<vkCreateDescriptorPool>,
    wrapper::options::handle<VkDescriptorPool>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyDescriptorPool>>;

using descriptor_pool = wrapper::decay_wrapper_t<unique_descriptor_pool>;
using unique_descriptor_set_layout = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::created>,
    wrapper::options::creator_call<vkCreateDescriptorSetLayout>,
    wrapper::options::handle<VkDescriptorSetLayout>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyDescriptorSetLayout>>;

using descriptor_set_layout =
    wrapper::decay_wrapper_t<unique_descriptor_set_layout>;

using unique_descriptor_set = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_free>,
    wrapper::options::creator<wrapper::creator::allocated>,
    wrapper::options::creator_call<vkAllocateDescriptorSets>,
    wrapper::options::handle<VkDescriptorSet>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::pool<VkDescriptorPool>,
    wrapper::options::deleter_call<vkFreeDescriptorSets>>;

using descriptor_set = wrapper::decay_wrapper_t<unique_descriptor_set>;

using unique_device_memory = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::created>,
    wrapper::options::creator_call<vkAllocateMemory>,
    wrapper::options::handle<VkDeviceMemory>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkFreeMemory>>;

using device_memory = wrapper::decay_wrapper_t<unique_device_memory>;
using queue = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkQueue>>;

using physical_device = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::handle_only>,
    wrapper::options::handle<VkPhysicalDevice>>;

using unique_device = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::owner_destroy>,
    wrapper::options::creator<wrapper::creator::created>,
    wrapper::options::creator_call<vkCreateDevice>,
    wrapper::options::handle<VkDevice>,
    wrapper::options::deleter_call<vkDestroyDevice>>;

using device = wrapper::decay_wrapper_t<unique_device>;

using unique_fence = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::created>,
    wrapper::options::creator_call<vkCreateFence>,
    wrapper::options::handle<VkFence>, wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyFence>>;

using fence = wrapper::decay_wrapper_t<unique_fence>;
using unique_framebuffer = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::created>,
    wrapper::options::creator_call<vkCreateFramebuffer>,
    wrapper::options::handle<VkFramebuffer>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyFramebuffer>>;

using framebuffer = wrapper::decay_wrapper_t<unique_framebuffer>;
using unique_image_view = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::created>,
    wrapper::options::creator_call<vkCreateImageView>,
    wrapper::options::handle<VkImageView>, wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyImageView>>;

using image_view = wrapper::decay_wrapper_t<unique_image_view>;
using unique_instance = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::owner_destroy>,
    wrapper::options::creator<wrapper::creator::created>,
    wrapper::options::creator_call<vkCreateInstance>,
    wrapper::options::handle<VkInstance>,
    wrapper::options::deleter_call<vkDestroyInstance>>;

using instance = wrapper::decay_wrapper_t<unique_instance>;
using unique_pipeline = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::created>,
    wrapper::options::creator_call<vkCreateGraphicsPipelines>,
    wrapper::options::handle<VkPipeline>, wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyPipeline>>;

using pipeline = wrapper::decay_wrapper_t<unique_pipeline>;

using unique_render_pass = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::created>,
    wrapper::options::creator_call<vkCreateRenderPass>,
    wrapper::options::handle<VkRenderPass>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyRenderPass>>;

using render_pass = wrapper::decay_wrapper_t<unique_render_pass>;

using unique_sampler = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::created>,
    wrapper::options::creator_call<vkCreateSampler>,
    wrapper::options::handle<VkSampler>, wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroySampler>>;

using sampler = wrapper::decay_wrapper_t<unique_sampler>;

using unique_semaphore = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::created>,
    wrapper::options::creator_call<vkCreateSemaphore>,
    wrapper::options::handle<VkSemaphore>, wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroySemaphore>>;

using semaphore = wrapper::decay_wrapper_t<unique_semaphore>;
using unique_surface = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::handle<VkSurfaceKHR>,
    wrapper::options::parent<VkInstance>,
    wrapper::options::deleter_call<vkDestroySurfaceKHR>>;

using surface = wrapper::decay_wrapper_t<unique_surface>;
using unique_image = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::created>,
    wrapper::options::creator_call<vkCreateImage>,
    wrapper::options::handle<VkImage>, wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyImage>>;

using image = wrapper::decay_wrapper_t<unique_image>;

using unique_shader_module = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::created>,
    wrapper::options::creator_call<vkCreateShaderModule>,
    wrapper::options::handle<VkShaderModule>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroyShaderModule>>;

using shader_module = wrapper::decay_wrapper_t<unique_shader_module>;

using unique_swapchain = wrapper::any_wrapper<
    wrapper::options::storage<wrapper::storage::unique>,
    wrapper::options::deleter<wrapper::deleter::object_destroy>,
    wrapper::options::creator<wrapper::creator::created>,
    wrapper::options::creator_call<vkCreateSwapchainKHR>,
    wrapper::options::handle<VkSwapchainKHR>,
    wrapper::options::parent<VkDevice>,
    wrapper::options::deleter_call<vkDestroySwapchainKHR>>;

using swapchain = wrapper::decay_wrapper_t<unique_swapchain>;

} // namespace mvk::types

#endif
