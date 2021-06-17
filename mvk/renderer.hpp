#ifndef MVK_RENDERER_HPP_INCLUDED
#define MVK_RENDERER_HPP_INCLUDED

#include "buffer_manager.hpp"
#include "types/types.hpp"

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
  record_commands();

  void
  recreate_after_framebuffer_change();

  static constexpr auto max_frames_in_flight = 2;

  types::window window_;

  // init_vulkan
  types::instance instance_;
  types::surface surface_;
  types::debug_messenger debug_messenger_;
  types::device device_;
  types::command_pool command_pool_;

  // init_swapchain
  types::swapchain swapchain_;
  types::image depth_image_;
  types::device_memory depth_image_memory_;
  types::image_view depth_image_view_;

  // preload_stuff
  types::shader_stage_builder builder_;
  types::image::texture texture_;
  types::image image_;
  types::device_memory image_memory_;
  types::image_view image_view_;
  types::sampler sampler_;
  buffer_manager vertex_buffer_manager_;
  buffer_manager index_buffer_manager_;
  std::vector<vertex> vertices_;
  std::vector<uint32_t> indices_;

  // init_main_renderpass
  types::render_pass render_pass_;

  // init_framebuffers
  std::vector<types::framebuffer> framebuffers_;

  // init_commands
  types::command_buffers command_buffers_;

  // init_descriptors
  types::descriptor_set_layout descriptor_set_layout_;
  types::descriptor_pool descriptor_pool_;
  types::descriptor_sets descriptor_sets_;
  std::vector<types::buffer> uniform_buffers_;
  std::vector<types::device_memory> uniform_buffers_memory_;

  // init_pipeline
  types::pipeline_layout pipeline_layout_;
  types::pipeline pipeline_;

  // init_sync
  template <typename T>
  using frame_array = std::array<T, max_frames_in_flight>;

  frame_array<types::semaphore> image_available_semaphores_;
  frame_array<types::semaphore> render_finished_semaphores_;
  frame_array<types::fence> frame_in_flight_fences_;

  // vector of ptrs as optional references
  std::vector<types::fence *> image_in_flight_fences_;

  std::chrono::time_point<std::chrono::high_resolution_clock> start_time =
      std::chrono::high_resolution_clock::now();
};

} // namespace mvk

#endif
