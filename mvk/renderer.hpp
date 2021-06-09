#ifndef MVK_RENDERER_HPP_INCLUDED
#define MVK_RENDERER_HPP_INCLUDED

#include "buffer_manager.hpp"
#include "vk_types/vk_types.hpp"

namespace mvk
{

struct renderer_context
{
  constexpr renderer_context() noexcept = default;
  renderer_context(int width, int height);

  vk_types::window          window_;
  vk_types::instance        instance_;
  vk_types::surface         surface_;
  vk_types::debug_messenger debug_messenger_;
  vk_types::device          device_;
  vk_types::command_pool    command_pool_;
};

struct swapchain_context
{
  swapchain_context() noexcept = default;

  swapchain_context(
    renderer_context const &               ctx,
    vk_types::sampler const &              sampler,
    vk_types::image_view const &           image_view,
    vk_types::shader_stage_builder const & builder,
    vk_types::buffer const &               vertex_buffer,
    vk_types::buffer const &               index_buffer,
    uint32_t                               indices_size,
    VkDeviceSize                           offset);

  vk_types::swapchain                  swapchain_;
  std::vector<vk_types::image_view>    image_views_;
  vk_types::render_pass                render_pass_;
  vk_types::descriptor_set_layout      descriptor_set_layout_;
  vk_types::pipeline_layout            pipeline_layout_;
  vk_types::pipeline                   pipeline_;
  vk_types::image                      depth_image_;
  vk_types::device_memory              depth_image_memory_;
  vk_types::image_view                 depth_image_view_;
  std::vector<vk_types::framebuffer>   framebuffers_;
  std::vector<vk_types::buffer>        uniform_buffers_;
  std::vector<vk_types::device_memory> uniform_buffers_memory_;
  vk_types::descriptor_pool            descriptor_pool_;
  vk_types::descriptor_sets            descriptor_sets_;
  vk_types::command_buffers            command_buffers_;
};

class renderer
{
public:
  renderer(int width, int height);

  void
  run();

private:
  static constexpr auto max_frames_in_flight = 2;

  void
  recreate_swapchain();

  renderer_context  ctx_;
  swapchain_context swp_ctx_;

  vk_types::shader_stage_builder builder_;

  template <typename T>
  using max_frames_in_flight_array = std::array<T, max_frames_in_flight>;

  max_frames_in_flight_array<vk_types::semaphore> image_available_semaphores_;
  max_frames_in_flight_array<vk_types::semaphore> render_finished_semaphores_;
  max_frames_in_flight_array<vk_types::fence>     frame_in_flight_fences_;

  std::vector<vk_types::fence *> image_in_flight_fences_;

  std::vector<vertex>   vertices_;
  std::vector<uint32_t> indices_;

  buffer_manager vertex_buffer_manager_;

  vk_types::buffer        vertex_buffer_;
  vk_types::device_memory vertex_buffer_memory_;

  vk_types::buffer        index_buffer_;
  vk_types::device_memory index_buffer_memory_;

  vk_types::image::texture texture_;
  vk_types::image          image_;
  vk_types::device_memory  image_memory_;
  vk_types::image_view     image_view_;

  vk_types::sampler sampler_;

  std::chrono::time_point<std::chrono::high_resolution_clock> start_time =
    std::chrono::high_resolution_clock::now();
};

} // namespace mvk

#endif
