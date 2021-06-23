#ifndef MVK_RENDERER_HPP_INCLUDED
#define MVK_RENDERER_HPP_INCLUDED

#include "buffer_manager.hpp"
#include "detail/helpers.hpp"
#include "shader_types.hpp"
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
  static constexpr auto device_extensions =
      std::array{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

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
  basic_draw(utility::slice<std::byte const> vertices,
             utility::slice<std::byte const> indices,
             utility::slice<std::byte const> pvm);

  [[nodiscard]] pvm
  create_test_pvm();

  [[nodiscard]] float
  time() const noexcept;

  static constexpr auto max_frames_in_flight = 2;

  types::window window_;

  // init_vulkan
  types::unique_instance instance_;
  types::unique_surface surface_;
  types::unique_debug_messenger debug_messenger_;
  types::physical_device physical_device_;
  types::unique_device device_;
  types::unique_command_pool command_pool_;
  u32 graphics_queue_index_ = {};
  u32 present_queue_index_ = {};
  types::queue graphics_queue_;
  types::queue present_queue_;

  // init_swapchain
  types::unique_swapchain swapchain_;
  std::vector<VkImage> swapchain_images_;
  std::vector<types::unique_image_view> swapchain_image_views_;
  types::unique_image depth_image_;
  types::unique_device_memory depth_image_memory_;
  types::unique_image_view depth_image_view_;
  VkExtent2D extent_ = {};

  // preload_stuff
  shader_stage_builder builder_;
  std::vector<unsigned char> texture_;
  u32 width_ = {};
  u32 height_ = {};
  types::unique_image image_;
  types::unique_device_memory image_memory_;
  types::unique_image_view image_view_;
  types::unique_sampler sampler_;
  buffer_manager vertex_buffer_manager_;
  buffer_manager index_buffer_manager_;
  std::vector<vertex> vertices_;
  std::vector<u32> indices_;

  // init_main_renderpass
  types::unique_render_pass render_pass_;

  // init_framebuffers
  std::vector<types::unique_framebuffer> framebuffers_;

  // init_commands
  std::vector<types::unique_command_buffer> command_buffers_;

  // init_descriptors
  types::unique_descriptor_set_layout descriptor_set_layout_;
  types::unique_descriptor_pool descriptor_pool_;
  std::vector<types::unique_descriptor_set> descriptor_sets_;
  std::vector<types::unique_buffer> uniform_buffers_;
  std::vector<types::unique_device_memory> uniform_buffers_memory_;
  std::vector<utility::slice<std::byte>> mapped_datas_;

  // init_pipeline
  types::unique_pipeline_layout pipeline_layout_;
  types::unique_pipeline pipeline_;

  // init_sync
  template <typename T>
  using frame_array = std::array<T, max_frames_in_flight>;

  frame_array<types::unique_semaphore> image_available_semaphores_;
  frame_array<types::unique_semaphore> render_finished_semaphores_;
  frame_array<types::unique_fence> frame_in_flight_fences_;

  // vector of ptrs as optional references
  std::vector<types::unique_fence *> image_in_flight_fences_;

  // rendering fino
  size_t current_frame_index_ = 0;
  u32 current_image_index_ = 0;
  types::command_buffer current_command_buffer_ = VK_NULL_HANDLE;

  std::chrono::time_point<std::chrono::high_resolution_clock> start_time =
      std::chrono::high_resolution_clock::now();
};

} // namespace mvk

#endif
