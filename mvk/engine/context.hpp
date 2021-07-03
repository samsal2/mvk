#ifndef MVK_ENGINE_CONTEXT_HPP_INCLUDED
#define MVK_ENGINE_CONTEXT_HPP_INCLUDED

#include "shader_types.hpp"
#include "types/types.hpp"
#include "utility/slice.hpp"

#include <array>
#include <vector>

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

    static constexpr auto                                                           max_frames_in_flight = 2;
    static constexpr size_t                                                         dynamic_buffer_count = 2;
    static constexpr size_t                                                         garbage_buffer_count = 2;
    //
    // init_window
    types::window                                                                   window_;
    //
    // init_instance
    types::unique_instance                                                          instance_;
    //
    // init_surface
    types::unique_surface                                                           surface_;
    VkSurfaceFormatKHR                                                              surface_format_;
    //
    // init_debug_messenger
    types::unique_debug_messenger                                                   debug_messenger_;
    //
    // init_device
    types::physical_device                                                          physical_device_;
    types::unique_device                                                            device_;
    uint32_t                                                                        graphics_queue_index_;
    uint32_t                                                                        present_queue_index_;
    types::queue                                                                    graphics_queue_;
    types::queue                                                                    present_queue_;
    //
    // init_layouts
    types::unique_descriptor_set_layout                                             uniform_descriptor_set_layout_;
    types::unique_descriptor_set_layout                                             texture_descriptor_set_layout_;
    //
    // pipeline_layouts
    types::unique_pipeline_layout                                                   pipeline_layout_;
    //
    // init_pools
    types::unique_command_pool                                                      command_pool_;
    types::unique_descriptor_pool                                                   descriptor_pool_;
    //
    // init_swapchain
    uint32_t                                                                        swapchain_images_count_;
    types::unique_swapchain                                                         swapchain_;
    std::vector< types::unique_image_view >                                         swapchain_image_views_;
    VkExtent2D                                                                      swapchain_extent_;
    //
    // init_depth_image
    types::unique_image                                                             depth_image_;
    types::unique_device_memory                                                     depth_image_memory_;
    types::unique_image_view                                                        depth_image_view_;
    //
    // init_framebuffers
    std::vector< types::unique_framebuffer >                                        framebuffers_;
    //
    // init_main_renderpass
    types::unique_render_pass                                                       render_pass_;
    //
    // doesnt belong here
    // ================================================================================================================
    std::vector< unsigned char >                                                    texture_;
    uint32_t                                                                        width_;
    uint32_t                                                                        height_;
    types::unique_image                                                             image_;
    types::unique_device_memory                                                     image_memory_;
    types::unique_image_view                                                        image_view_;
    types::unique_descriptor_set                                                    image_descriptor_set_;
    std::vector< vertex >                                                           vertices_;
    std::vector< uint32_t >                                                         indices_;
    // ================================================================================================================
    //
    // allocate_commands
    std::array< types::unique_command_buffer, dynamic_buffer_count >                command_buffers_;
    //
    // shaders
    types::unique_shader_module                                                     vertex_shader_;
    types::unique_shader_module                                                     fragment_shader_;
    //
    // samplers
    types::unique_sampler                                                           texture_sampler_;
    //
    // init_pipeline
    types::unique_pipeline                                                          pipeline_;
    //
    // init_sync
    std::array< types::unique_semaphore, max_frames_in_flight >                     image_available_semaphores_;
    std::array< types::unique_semaphore, max_frames_in_flight >                     render_finished_semaphores_;
    std::array< types::unique_fence, max_frames_in_flight >                         frame_in_flight_fences_;
    std::vector< types::unique_fence * >                                            image_in_flight_fences_;
    //
    // buffers
    // vertex
    VkMemoryRequirements                                                            vertex_memory_requirements_;
    types::device_size                                                              vertex_aligned_size_;
    std::array< types::unique_buffer, dynamic_buffer_count >                        vertex_buffers_;
    std::array< types::device_size, dynamic_buffer_count >                          vertex_offsets_;
    types::unique_device_memory                                                     vertex_memory_;
    //
    // index
    VkMemoryRequirements                                                            index_memory_requirements_;
    types::device_size                                                              index_aligned_size_;
    std::array< types::unique_buffer, dynamic_buffer_count >                        index_buffers_;
    std::array< types::device_size, dynamic_buffer_count >                          index_offsets_;
    types::unique_device_memory                                                     index_memory_;
    //
    // staging
    VkMemoryRequirements                                                            staging_memory_requirements_;
    types::device_size                                                              staging_aligned_size_;
    std::array< types::unique_buffer, dynamic_buffer_count >                        staging_buffers_;
    std::array< types::device_size, dynamic_buffer_count >                          staging_offsets_;
    types::unique_device_memory                                                     staging_memory_;
    utility::slice< std::byte >                                                     staging_data_;
    //
    // ubo
    VkMemoryRequirements                                                            uniform_memory_requirements_;
    types::device_size                                                              uniform_aligned_size_;
    std::array< types::unique_buffer, dynamic_buffer_count >                        uniform_buffers_;
    std::array< uint32_t, dynamic_buffer_count >                                    uniform_offsets_;
    std::array< types::unique_descriptor_set, dynamic_buffer_count >                uniform_descriptor_sets_;
    types::unique_device_memory                                                     uniform_memory_;
    utility::slice< std::byte >                                                     uniform_data_;
    //
    // garbage
    std::array< std::vector< types::unique_buffer >, garbage_buffer_count >         garbage_buffers_;
    std::array< std::vector< types::unique_device_memory >, garbage_buffer_count >  garbage_memories_;
    std::array< std::vector< types::unique_descriptor_set >, garbage_buffer_count > garbage_descriptor_sets_;
    //
    // rendering counters
    size_t                                                                          current_frame_index_   = 0;
    size_t                                                                          current_buffer_index_  = 0;
    size_t                                                                          current_garbage_index_ = 0;
    uint32_t                                                                        current_image_index_   = 0;
    types::command_buffer                                         current_command_buffer_ = VK_NULL_HANDLE;
    //
    std::chrono::time_point< std::chrono::high_resolution_clock > start_time =
      std::chrono::high_resolution_clock::now();
  };

  [[nodiscard]] float current_time( context const & ctx ) noexcept;

}  // namespace mvk::engine

#endif
