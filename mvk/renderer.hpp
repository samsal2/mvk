#ifndef MVK_RENDERER_HPP_INCLUDED
#define MVK_RENDERER_HPP_INCLUDED

#include "buffer_manager.hpp"
#include "vk_types/vk_types.hpp"

namespace mvk
{

class renderer
{
public:
    renderer() = default;

    void
    init();

    void
    run();

private:
    void
    init_vulkan();

    void
    init_swapchain();

    void
    preload_stuff();

    void
    init_main_renderpass();

    void
    init_framebuffers();

    void
    init_commands();

    void
    init_descriptors();

    void
    init_pipeline();

    void
    init_sync();

    void
    load_mesh();

    void
    recreate_after_framebuffer_change();

    static constexpr auto max_frames_in_flight = 2;

    vk_types::window window_;

    // init_vulkan
    vk_types::instance        instance_;
    vk_types::surface         surface_;
    vk_types::debug_messenger debug_messenger_;
    vk_types::device          device_;
    vk_types::command_pool    command_pool_;

    // init_swapchain
    vk_types::swapchain     swapchain_;
    vk_types::image         depth_image_;
    vk_types::device_memory depth_image_memory_;
    vk_types::image_view    depth_image_view_;

    // preload_stuff
    vk_types::shader_stage_builder builder_;
    vk_types::image::texture       texture_;
    vk_types::image                image_;
    vk_types::device_memory        image_memory_;
    vk_types::image_view           image_view_;
    vk_types::sampler              sampler_;
    buffer_manager                 vertex_buffer_manager_;
    buffer_manager                 index_buffer_manager_;
    std::vector<vertex>            vertices_;
    std::vector<uint32_t>          indices_;

    // init_main_renderpass
    vk_types::render_pass render_pass_;

    // init_framebuffers
    std::vector<vk_types::framebuffer> framebuffers_;

    // init_commands
    vk_types::command_buffers command_buffers_;

    // init_descriptors
    vk_types::descriptor_set_layout      descriptor_set_layout_;
    vk_types::descriptor_pool            descriptor_pool_;
    vk_types::descriptor_sets            descriptor_sets_;
    std::vector<vk_types::buffer>        uniform_buffers_;
    std::vector<vk_types::device_memory> uniform_buffers_memory_;

    // init_pipeline
    vk_types::pipeline_layout pipeline_layout_;
    vk_types::pipeline        pipeline_;

    // init_sync
    std::array<vk_types::semaphore, max_frames_in_flight> image_available_semaphores_;
    std::array<vk_types::semaphore, max_frames_in_flight> render_finished_semaphores_;
    std::array<vk_types::fence, max_frames_in_flight>     frame_in_flight_fences_;

    // vector of ptrs as optional references
    std::vector<vk_types::fence *> image_in_flight_fences_;

    std::chrono::time_point<std::chrono::high_resolution_clock> start_time = std::chrono::high_resolution_clock::now();
};

} // namespace mvk

#endif
