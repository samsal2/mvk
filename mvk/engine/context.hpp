#ifndef MVK_ENGINE_CONTEXT_HPP_INCLUDED
#define MVK_ENGINE_CONTEXT_HPP_INCLUDED

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "shader_types.hpp"
#include "utility/slice.hpp"

#include <array>
#include <vector>
#include <vulkan/vulkan.h>

namespace mvk::engine
{
  struct context
  {
#ifndef NDEBUG
    static constexpr auto use_validation = true;
#else
    static constexpr auto use_validation = false;
#endif

    static constexpr auto validation_layers              = std::array{ "VK_LAYER_KHRONOS_validation" };
    static constexpr auto validation_instance_extensions = std::array{ VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
    static constexpr auto device_extensions              = std::array{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    static constexpr auto                                              max_frames_in_flight = 2;
    static constexpr size_t                                            dynamic_buffer_count = 2;
    static constexpr size_t                                            garbage_buffer_count = 2;
    //
    // init_window
    GLFWwindow *                                                       window;
    bool                                                               framebuffer_resized;
    //
    // init_instance
    VkInstance                                                         instance;
    //
    // init_surface
    VkSurfaceKHR                                                       surface;
    VkSurfaceFormatKHR                                                 surface_format;
    //
    // init_debug_messenger
    VkDebugUtilsMessengerEXT                                           debug_messenger;
    //
    // init_device
    VkPhysicalDevice                                                   physical_device;
    VkDevice                                                           device;
    uint32_t                                                           graphics_queue_index;
    uint32_t                                                           present_queue_index;
    VkQueue                                                            graphics_queue;
    VkQueue                                                            present_queue;
    //
    // init_layouts
    VkDescriptorSetLayout                                              uniform_descriptor_set_layout;
    VkDescriptorSetLayout                                              texture_descriptor_set_layout;
    //
    // pipeline_layouts
    VkPipelineLayout                                                   pipeline_layout;
    //
    // init_pools
    VkCommandPool                                                      command_pool;
    VkDescriptorPool                                                   descriptor_pool;
    //
    // init_swapchain
    uint32_t                                                           swapchain_images_count;
    VkSwapchainKHR                                                     swapchain;
    std::vector< VkImageView >                                         swapchain_image_views;
    VkExtent2D                                                         swapchain_extent;
    //
    // init_depth_image
    VkImage                                                            depth_image;
    VkDeviceMemory                                                     depth_image_memory;
    VkImageView                                                        depth_image_view;
    //
    // init_framebuffers
    std::vector< VkFramebuffer >                                       framebuffers;
    //
    // init_main_renderpass
    VkRenderPass                                                       render_pass;
    //
    // doesnt belong here
    // ================================================================================================================
    std::vector< unsigned char >                                       texture_;
    uint32_t                                                           width_;
    uint32_t                                                           height_;
    VkImage                                                            image_;
    VkDeviceMemory                                                     image_memory_;
    VkImageView                                                        image_view_;
    VkDescriptorSet                                                    image_descriptor_set_;
    std::vector< vertex >                                              vertices_;
    std::vector< uint32_t >                                            indices_;
    // ================================================================================================================
    //
    // allocate_commands
    std::array< VkCommandBuffer, dynamic_buffer_count >                command_buffers;
    //
    // shaders
    VkShaderModule                                                     vertex_shader;
    VkShaderModule                                                     fragment_shader;
    //
    // samplers
    VkSampler                                                          texture_sampler;
    //
    // init_pipeline
    VkPipeline                                                         pipeline;
    //
    // init_sync
    std::array< VkSemaphore, max_frames_in_flight >                    image_available_semaphores;
    std::array< VkSemaphore, max_frames_in_flight >                    render_finished_semaphores;
    std::array< VkFence, max_frames_in_flight >                        frame_in_flight_fences;
    std::vector< VkFence * >                                           image_in_flight_fences;
    //
    // buffers
    // vertex
    VkMemoryRequirements                                               vertex_memory_requirements;
    VkDeviceSize                                                       vertex_aligned_size;
    std::array< VkBuffer, dynamic_buffer_count >                       vertex_buffers;
    std::array< VkDeviceSize, dynamic_buffer_count >                   vertex_offsets;
    VkDeviceMemory                                                     vertex_memory;
    //
    // index
    VkMemoryRequirements                                               index_memory_requirements;
    VkDeviceSize                                                       index_aligned_size;
    std::array< VkBuffer, dynamic_buffer_count >                       index_buffers;
    std::array< VkDeviceSize, dynamic_buffer_count >                   index_offsets;
    VkDeviceMemory                                                     index_memory;
    //
    // staging
    VkMemoryRequirements                                               staging_memory_requirements;
    VkDeviceSize                                                       staging_aligned_size;
    std::array< VkBuffer, dynamic_buffer_count >                       staging_buffers;
    std::array< VkDeviceSize, dynamic_buffer_count >                   staging_offsets;
    VkDeviceMemory                                                     staging_memory;
    utility::slice< std::byte >                                        staging_data;
    //
    // ubo
    VkMemoryRequirements                                               uniform_memory_requirements;
    VkDeviceSize                                                       uniform_aligned_size;
    std::array< VkBuffer, dynamic_buffer_count >                       uniform_buffers;
    std::array< uint32_t, dynamic_buffer_count >                       uniform_offsets;
    std::array< VkDescriptorSet, dynamic_buffer_count >                uniform_descriptor_sets;
    VkDeviceMemory                                                     uniform_memory;
    utility::slice< std::byte >                                        uniform_data;
    //
    // garbage
    std::array< std::vector< VkBuffer >, garbage_buffer_count >        garbage_buffers;
    std::array< std::vector< VkDeviceMemory >, garbage_buffer_count >  garbage_memories;
    std::array< std::vector< VkDescriptorSet >, garbage_buffer_count > garbage_descriptor_sets;
    //
    // rendering counters
    size_t                                                             current_frame_index    = 0;
    size_t                                                             current_buffer_index   = 0;
    size_t                                                             current_garbage_index  = 0;
    uint32_t                                                           current_image_index    = 0;
    VkCommandBuffer                                                    current_command_buffer = VK_NULL_HANDLE;
    //
    std::chrono::time_point< std::chrono::high_resolution_clock >      start_time =
      std::chrono::high_resolution_clock::now();
  };

  [[nodiscard]] float current_time( context const & ctx ) noexcept;

  [[nodiscard]] VkExtent2D query_framebuffer_size( context const & ctx ) noexcept;

}  // namespace mvk::engine

#endif
