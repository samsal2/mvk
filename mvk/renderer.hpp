#ifndef MVK_RENDERER_HPP_INCLUDED
#define MVK_RENDERER_HPP_INCLUDED

#include "buffer_manager.hpp"
#include "detail/helpers.hpp"
#include "types/types.hpp"
#include <span>

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

  void
  begin_draw();

  void
  end_draw();

  void
  basic_draw(utility::slice<std::byte> vertices,
             utility::slice<std::byte> indices,
             utility::slice<std::byte> pvm);

  [[nodiscard]] pvm
  create_test_pvm();

  [[nodiscard]] float
  time() const noexcept;

  static constexpr auto max_frames_in_flight = 2;

  types::window window_;

  // init_vulkan
  types::instance instance_;
  types::surface surface_;
  types::debug_messenger debug_messenger_;
  types::physical_device physical_device_;
  types::device device_;
  types::command_pool command_pool_;
  uint32_t graphics_queue_index_;
  uint32_t present_queue_index_;
  types::queue graphics_queue_;
  types::queue present_queue_;

  // init_swapchain
  types::swapchain swapchain_;
  std::vector<VkImage> swapchain_images_;
  std::vector<types::image_view> swapchain_image_views_;
  types::image depth_image_;
  types::device_memory depth_image_memory_;
  types::image_view depth_image_view_;
  VkExtent2D extent_;

  // preload_stuff
  shader_stage_builder builder_;
  std::vector<unsigned char> texture_;
  uint32_t width_;
  uint32_t height_;
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
  std::vector<std::span<std::byte>> mapped_datas_;

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

  // rendering fino
  size_t current_frame_index_ = 0;
  uint32_t current_image_index_ = 0;

  std::chrono::time_point<std::chrono::high_resolution_clock> start_time =
      std::chrono::high_resolution_clock::now();
};

} // namespace mvk

#endif
