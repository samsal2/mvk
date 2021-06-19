#ifndef MVK_TYPES_TMP_HPP_INCLUDED
#define MVK_TYPES_TMP_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"
#include "types/validation/validation.hpp"
#include "types/window.hpp"

namespace mvk::types
{

using buffer =
    detail::wrapper<detail::creator<vkCreateBuffer>, detail::handle<VkBuffer>,
                    detail::parent<VkDevice>,
                    detail::deleter<vkDestroyBuffer>>;

using pipeline_layout = detail::wrapper<
    detail::creator<vkCreatePipelineLayout>, detail::handle<VkPipelineLayout>,
    detail::parent<VkDevice>, detail::deleter<vkDestroyPipelineLayout>>;

using command_buffers =
    detail::wrapper<detail::creator<vkAllocateCommandBuffers>,
                    detail::handle<std::vector<VkCommandBuffer>>,
                    detail::parent<VkDevice>, detail::pool<VkCommandPool>,
                    detail::deleter<vkFreeCommandBuffers>>;

using command_buffer = detail::wrapper<detail::handle<VkCommandBuffer>>;

using command_pool =
    detail::wrapper<detail::creator<vkCreateCommandPool>,
                    detail::handle<VkCommandPool>, detail::parent<VkDevice>,
                    detail::deleter<vkDestroyCommandPool>>;

using debug_messenger =
    detail::wrapper<detail::creator<validation::setup_debug_messenger>,
                    detail::handle<VkDebugUtilsMessengerEXT>,
                    detail::parent<VkInstance>,
                    detail::deleter<validation::destroy_debug_messenger>>;

using descriptor_pool = detail::wrapper<
    detail::creator<vkCreateDescriptorPool>, detail::handle<VkDescriptorPool>,
    detail::parent<VkDevice>, detail::deleter<vkDestroyDescriptorPool>>;

using descriptor_set_layout =
    detail::wrapper<detail::creator<vkCreateDescriptorSetLayout>,
                    detail::handle<VkDescriptorSetLayout>,
                    detail::parent<VkDevice>,
                    detail::deleter<vkDestroyDescriptorSetLayout>>;

using descriptor_sets =
    detail::wrapper<detail::creator<vkAllocateDescriptorSets>,
                    detail::handle<std::vector<VkDescriptorSet>>,
                    detail::parent<VkDevice>, detail::pool<VkDescriptorPool>,
                    detail::deleter<vkFreeDescriptorSets>>;

using descriptor_set = detail::wrapper<detail::handle<VkDescriptorSet>>;

using device_memory =
    detail::wrapper<detail::creator<vkAllocateMemory>,
                    detail::handle<VkDeviceMemory>, detail::parent<VkDevice>,
                    detail::deleter<vkFreeMemory>>;

using queue = detail::wrapper<detail::creator<vkGetDeviceQueue>,
                              detail::handle<VkQueue>>;

using physical_device = detail::wrapper<detail::handle<VkPhysicalDevice>>;

using device =
    detail::wrapper<detail::creator<vkCreateDevice>, detail::handle<VkDevice>,
                    detail::deleter<vkDestroyDevice>>;

using fence =
    detail::wrapper<detail::creator<vkCreateFence>, detail::handle<VkFence>,
                    detail::parent<VkDevice>,
                    detail::deleter<vkDestroyFence>>;

using framebuffer =
    detail::wrapper<detail::creator<vkCreateFramebuffer>,
                    detail::handle<VkFramebuffer>, detail::parent<VkDevice>,
                    detail::deleter<vkDestroyFramebuffer>>;

using image_view =
    detail::wrapper<detail::creator<vkCreateImageView>,
                    detail::handle<VkImageView>, detail::parent<VkDevice>,
                    detail::deleter<vkDestroyImageView>>;

using instance = detail::wrapper<detail::creator<vkCreateInstance>,
                                 detail::handle<VkInstance>,
                                 detail::deleter<vkDestroyInstance>>;

using pipeline =
    detail::wrapper<detail::creator<vkCreateGraphicsPipelines>,
                    detail::handle<VkPipeline>, detail::parent<VkDevice>,
                    detail::deleter<vkDestroyPipeline>>;

using render_pass =
    detail::wrapper<detail::creator<vkCreateRenderPass>,
                    detail::handle<VkRenderPass>, detail::parent<VkDevice>,
                    detail::deleter<vkDestroyRenderPass>>;

using sampler =
    detail::wrapper<detail::creator<vkCreateSampler>,
                    detail::handle<VkSampler>, detail::parent<VkDevice>,
                    detail::deleter<vkDestroySampler>>;

using semaphore =
    detail::wrapper<detail::creator<vkCreateSemaphore>,
                    detail::handle<VkSemaphore>, detail::parent<VkDevice>,
                    detail::deleter<vkDestroySemaphore>>;

using surface =
    detail::wrapper<detail::creator<glfwCreateWindowSurface>,
                    detail::handle<VkSurfaceKHR>, detail::parent<VkInstance>,
                    detail::deleter<vkDestroySurfaceKHR>>;

using image =
    detail::wrapper<detail::creator<vkCreateImage>, detail::handle<VkImage>,
                    detail::parent<VkDevice>,
                    detail::deleter<vkDestroyImage>>;

using shader_module =
    detail::wrapper<detail::creator<vkCreateShaderModule>,
                    detail::handle<VkShaderModule>, detail::parent<VkDevice>,
                    detail::deleter<vkDestroyShaderModule>>;

using swapchain =
    detail::wrapper<detail::creator<vkCreateSwapchainKHR>,
                    detail::handle<VkSwapchainKHR>, detail::parent<VkDevice>,
                    detail::deleter<vkDestroySwapchainKHR>>;

} // namespace mvk::types

#endif
