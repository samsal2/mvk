#include "renderer.hpp"

#include "factories.hpp"
#include "utility/misc.hpp"
#include "utility/slice.hpp"
#include "utility/verify.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#pragma clang diagnostic pop

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace mvk
{

namespace detail
{

[[nodiscard]] static std::pair<std::vector<vertex>, std::vector<uint32_t>>
load_obj(std::filesystem::path const & path)
{
  auto attrib    = tinyobj::attrib_t();
  auto shapes    = std::vector<tinyobj::shape_t>();
  auto materials = std::vector<tinyobj::material_t>();
  auto warn      = std::string();
  auto error     = std::string();

  [[maybe_unused]] auto const success = tinyobj::LoadObj(
    &attrib,
    &shapes,
    &materials,
    &warn,
    &error,
    path.c_str());

  MVK_VERIFY(success);

  auto vertices = std::vector<vertex>();
  auto indices  = std::vector<uint32_t>();

  for (auto const & shape : shapes)
  {
    for (auto const & index : shape.mesh.indices)
    {
      indices.push_back(static_cast<uint32_t>(std::size(indices)));

      auto       vtx          = vertex();
      auto const vertex_index = static_cast<size_t>(index.vertex_index);

      vtx.pos = [&attrib, vertex_index]
      {
        auto const x = attrib.vertices[3 * vertex_index + 0];
        auto const y = attrib.vertices[3 * vertex_index + 1];
        auto const z = attrib.vertices[3 * vertex_index + 2];
        return glm::vec3(x, y, z);
      }();

      vtx.color = glm::vec3(1.0F, 1.0F, 1.0F);

      auto const texture_coordinates_index =
        static_cast<size_t>(index.texcoord_index);

      vtx.texture_coord = [&attrib, &texture_coordinates_index]
      {
        auto const x = attrib.texcoords[2 * texture_coordinates_index + 0];
        auto const y = attrib.texcoords[2 * texture_coordinates_index + 1];
        return glm::vec2(x, 1 - y);
      }();

      vertices.push_back(vtx);
    }
  }

  return std::make_pair(vertices, indices);
}

[[nodiscard]] static std::vector<char>
read_file(std::filesystem::path const & path)
{
  MVK_VERIFY(std::filesystem::exists(path));
  auto file   = std::ifstream(path, std::ios::ate | std::ios::binary);
  auto buffer = std::vector<char>(static_cast<size_t>(file.tellg()));

  file.seekg(0);
  auto [data, size] = utility::bind_data_and_size(buffer);
  file.read(data, static_cast<int64_t>(size));
  file.close();

  return buffer;
}

} // namespace detail

renderer_context::renderer_context(int const width, int const height)
{
  MVK_VERIFY(vk_types::validation::check_support());

  window_ = vk_types::window(width, height);

  auto const required_extensions = window_.required_extensions();

  instance_ = factories::create_instance("stan loona", required_extensions);
  surface_  = factories::create_surface(instance_, window_.get());
  debug_messenger_ = vk_types::debug_messenger(instance_.get());

  constexpr auto device_extensions =
    std::array{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  device_       = factories::create_device(surface_, device_extensions);
  command_pool_ = factories::create_command_pool(device_);
}

swapchain_context::swapchain_context(
  renderer_context const &               ctx,
  vk_types::sampler const &              sampler,
  vk_types::image_view const &           image_view,
  vk_types::shader_stage_builder const & builder,
  vk_types::buffer const &               vertex_buffer,
  vk_types::buffer const &               index_buffer,
  uint32_t const                         indices_size,
  VkDeviceSize const                     offset)
{
  auto width  = 0;
  auto height = 0;
  glfwGetFramebufferSize(ctx.window_.get(), &width, &height);

  while (width == 0 || height == 0)
  {
    glfwGetFramebufferSize(ctx.window_.get(), &width, &height);
    glfwWaitEvents();
  }

  ctx.device_.wait_idle();

  swapchain_ = factories::create_swapchain(
    ctx.device_,
    ctx.surface_,
    {static_cast<uint32_t>(width), static_cast<uint32_t>(height)});

  auto const & images      = swapchain_.images();
  auto const   images_size = std::size(images);
  render_pass_ = factories::create_render_pass(ctx.device_, ctx.surface_);

  image_views_.reserve(std::size(images));

  auto const add_image_view = [this, &ctx](auto const & image)
  {
    image_views_.push_back(
      factories::create_image_view(ctx.device_, ctx.surface_, image));
  };
  std::for_each(std::begin(images), std::end(images), add_image_view);

  descriptor_set_layout_ =
    factories::create_descriptor_set_layout(ctx.device_);

  pipeline_layout_ = factories::create_pipeline_layout(descriptor_set_layout_);

  pipeline_ = factories::create_pipeline(
    swapchain_,
    render_pass_,
    pipeline_layout_,
    builder.stages());

  depth_image_ = factories::create_image(
    ctx.device_,
    swapchain_.extent().width,
    swapchain_.extent().height,
    VK_FORMAT_D32_SFLOAT,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    VK_SHARING_MODE_EXCLUSIVE);

  depth_image_memory_ = factories::create_device_memory(
    ctx.device_,
    depth_image_,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  depth_image_view_ = factories::create_image_view(
    depth_image_,
    VK_FORMAT_D32_SFLOAT,
    VK_IMAGE_ASPECT_DEPTH_BIT);

  depth_image_.transition_layout(
    ctx.device_,
    ctx.command_pool_,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  framebuffers_.reserve(std::size(image_views_));

  auto const add_frame = [this](auto const & frame_image_view)
  {
    framebuffers_.push_back(factories::create_framebuffer(
      swapchain_,
      render_pass_,
      frame_image_view,
      depth_image_view_));
  };
  std::for_each(std::begin(image_views_), std::end(image_views_), add_frame);

  uniform_buffers_memory_.reserve(images_size);
  for (auto i = size_t(0); i < images_size; ++i)
  {
    uniform_buffers_.push_back(factories::create_buffer(
      ctx.device_,
      sizeof(pvm),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_SHARING_MODE_EXCLUSIVE));

    uniform_buffers_memory_.push_back(factories::create_device_memory(
      ctx.device_,
      uniform_buffers_.back(),
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

    uniform_buffers_memory_.back().map(sizeof(pvm));
  }

  descriptor_pool_ =
    factories::create_descriptor_pool(ctx.device_, images_size);

  descriptor_sets_ = factories::create_descriptor_sets(
    descriptor_pool_,
    descriptor_set_layout_,
    swapchain_);

  command_buffers_ = factories::create_command_buffers(
    ctx.command_pool_,
    static_cast<uint32_t>(std::size(framebuffers_)));

  for (auto i = size_t(0); i < images_size; ++i)
  {
    auto buffer_info   = VkDescriptorBufferInfo();
    buffer_info.buffer = uniform_buffers_[i].get();
    buffer_info.offset = 0;
    buffer_info.range  = sizeof(pvm);

    auto image_info        = VkDescriptorImageInfo();
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView   = image_view.get();
    image_info.sampler     = sampler.get();

    auto const descriptor_writes = std::array{
      [this, &buffer_info, i]
      {
        auto ubo             = VkWriteDescriptorSet();
        ubo.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        ubo.dstSet           = descriptor_sets_.get()[i];
        ubo.dstBinding       = 0;
        ubo.dstArrayElement  = 0;
        ubo.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubo.descriptorCount  = 1;
        ubo.pBufferInfo      = &buffer_info;
        ubo.pImageInfo       = nullptr;
        ubo.pTexelBufferView = nullptr;
        return ubo;
      }(),
      [this, &image_info, i]
      {
        auto image            = VkWriteDescriptorSet();
        image.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        image.dstSet          = descriptor_sets_.get()[i];
        image.dstBinding      = 1;
        image.dstArrayElement = 0;
        image.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        image.descriptorCount = 1;
        image.pImageInfo      = &image_info;
        return image;
      }()};

    auto const [write_data, write_size] =
      utility::bind_data_and_size(descriptor_writes);

    vkUpdateDescriptorSets(
      ctx.device_.get(),
      static_cast<uint32_t>(write_size),
      write_data,
      0,
      nullptr);
  }

  for (size_t i = 0; i < std::size(command_buffers_.get()); ++i)
  {
    auto clear_color_value  = VkClearValue();
    clear_color_value.color = {{0.0F, 0.0F, 0.0F, 1.0F}};

    auto clear_depth_value         = VkClearValue();
    clear_depth_value.depthStencil = {1.0F, 0};

    auto const clear_values = std::array{clear_color_value, clear_depth_value};

    auto const clear_values_size =
      static_cast<uint32_t>(std::size(clear_values));

    auto render_pass_begin_info  = VkRenderPassBeginInfo();
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass          = render_pass_.get();
    render_pass_begin_info.framebuffer         = framebuffers_[i].get();
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.renderArea.extent   = swapchain_.extent();
    render_pass_begin_info.clearValueCount     = clear_values_size;
    render_pass_begin_info.pClearValues        = std::data(clear_values);

    auto command_buffer_begin_info = VkCommandBufferBeginInfo();
    command_buffer_begin_info.sType =
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags            = 0;
    command_buffer_begin_info.pInheritanceInfo = nullptr;

    command_buffers_.begin(i, command_buffer_begin_info)
      .begin_render_pass(render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE)
      .bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.get())
      .bind_vertex_buffer(vertex_buffer.get(), {&offset, 1})
      .bind_index_buffer(index_buffer.get())
      .bind_descriptor_sets(
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout_.get(),
        0,
        1,
        descriptor_sets_.get()[i])
      .draw_indexed(indices_size, 1, 0, 0, 0)
      .end_render_pass()
      .end();
  }
}

void
renderer::recreate_swapchain()
{
  auto [vertex_buffer, offset] =
    vertex_buffer_manager_.map(utility::as_bytes(vertices_));

  swp_ctx_ = swapchain_context(
    ctx_,
    sampler_,
    image_view_,
    builder_,
    vertex_buffer,
    index_buffer_,
    static_cast<uint32_t>(std::size(indices_)),
    offset);
}

renderer::renderer(int const width, int const height) : ctx_(width, height)
{

  auto const vertex_code = detail::read_file("../../shaders/vert.spv");
  auto       vtx_shader  = factories::create_shader_module(
    ctx_.device_,
    {std::data(vertex_code), std::size(vertex_code)});

  auto const fragment_code = detail::read_file("../../shaders/frag.spv");
  auto       frag_shader   = factories::create_shader_module(
    ctx_.device_,
    {std::data(fragment_code), std::size(fragment_code)});

  builder_.add_stage(std::move(vtx_shader), VK_SHADER_STAGE_VERTEX_BIT)
    .add_stage(std::move(frag_shader), VK_SHADER_STAGE_FRAGMENT_BIT);

  for (auto i = size_t(0); i < max_frames_in_flight; ++i)
  {
    image_available_semaphores_.at(i) =
      factories::create_semaphore(ctx_.device_);
    render_finished_semaphores_.at(i) =
      factories::create_semaphore(ctx_.device_);
    frame_in_flight_fences_.at(i) = factories::create_fence(ctx_.device_);
  }

  std::tie(vertices_, indices_) =
    detail::load_obj("../../assets/viking_room.obj");

  vertex_buffer_manager_ =
    buffer_manager(&ctx_.device_, &ctx_.command_pool_, buffer_type::vertex);

  vertex_buffer_ = factories::create_buffer(
    ctx_.device_,
    std::size(utility::as_bytes(vertices_)),
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_SHARING_MODE_EXCLUSIVE);

  // This already binds device_memory
  vertex_buffer_memory_ = factories::create_device_memory(
    ctx_.device_,
    vertex_buffer_,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  vertex_buffer_.stage(
    ctx_.device_,
    ctx_.command_pool_,
    utility::as_bytes(vertices_));

  index_buffer_ = factories::create_buffer(
    ctx_.device_,
    std::size(utility::as_bytes(indices_)),
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    VK_SHARING_MODE_EXCLUSIVE);

  index_buffer_memory_ = factories::create_device_memory(
    ctx_.device_,
    index_buffer_,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  index_buffer_.stage(
    ctx_.device_,
    ctx_.command_pool_,
    utility::as_bytes(indices_));

  texture_ = vk_types::image::texture("../../assets/viking_room.png");

  image_ = factories::create_image(ctx_.device_, texture_);

  // NOTE(samuel): binds image_ to the memory created
  image_memory_ = factories::create_device_memory(
    ctx_.device_,
    image_,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  image_.transition_layout(
    ctx_.device_,
    ctx_.command_pool_,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  image_.stage(ctx_.device_, ctx_.command_pool_, texture_);

  image_.generate_mipmaps(
    ctx_.device_,
    ctx_.command_pool_,
    texture_.width(),
    texture_.height());

  image_view_ = factories::create_image_view(
    image_,
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_ASPECT_COLOR_BIT);

  sampler_ = factories::create_sampler(ctx_.device_, image_.mipmap_levels());

  // NOTE(samuel): must create swapchain before
  recreate_swapchain();

  image_in_flight_fences_.resize(
    std::size(swp_ctx_.swapchain_.images()),
    nullptr);
}

void
renderer::run()
{
  auto current_frame = size_t(0);

  while (glfwWindowShouldClose(ctx_.window_.get()) == 0)
  {
    glfwPollEvents();

    auto const & image_available_semaphore =
      image_available_semaphores_[current_frame];

    auto const & image_index =
      swp_ctx_.swapchain_.next_image(image_available_semaphore.get());

    if (!image_index.has_value())
    {
      recreate_swapchain();
      continue;
    }

    auto const current_index         = image_index.value();
    auto &     frame_in_flight_fence = frame_in_flight_fences_[current_frame];
    auto &     image_in_flight_fence = image_in_flight_fences_[current_index];

    if (image_in_flight_fence != nullptr)
    {
      image_in_flight_fence->wait();
    }

    image_in_flight_fence = &frame_in_flight_fence;

    auto const render_finished_semaphore =
      render_finished_semaphores_[current_frame].get();

    auto const wait_semaphores   = std::array{image_available_semaphore.get()};
    auto const signal_semaphores = std::array{render_finished_semaphore};
    auto const wait_stages       = std::array<VkPipelineStageFlags, 1>{
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    auto const swapchains   = std::array{swp_ctx_.swapchain_.get()};
    auto const current_time = std::chrono::high_resolution_clock::now();
    auto const delta_time   = current_time - start_time;
    auto const time =
      std::chrono::duration<float, std::chrono::seconds::period>(delta_time)
        .count();

    constexpr auto turn_rate = glm::radians(90.0F);

    auto ubo = pvm();

    ubo.model = glm::rotate(
      glm::mat4(1.0F),
      time * turn_rate,
      glm::vec3(0.0F, 0.0F, 1.0F));

    ubo.view = glm::lookAt(
      glm::vec3(2.0F, 2.0F, 2.0F),
      glm::vec3(0.0F, 0.0F, 0.0F),
      glm::vec3(0.0F, 0.0F, 1.0F));

    auto const ratio = static_cast<float>(swp_ctx_.swapchain_.extent().width) /
                       static_cast<float>(swp_ctx_.swapchain_.extent().height);

    ubo.proj = glm::perspective(glm::radians(45.0F), ratio, 0.1F, 10.0F);

    ubo.proj[1][1] *= -1;

    auto & current_ubo_memory =
      swp_ctx_.uniform_buffers_memory_[current_index];

    current_ubo_memory.copy_data(
      {utility::force_cast_to_byte(&ubo), sizeof(ubo)});

    auto const submit_info =
      [&wait_semaphores, &signal_semaphores, &wait_stages, this, current_index]
    {
      auto const wait_semaphore_count =
        static_cast<uint32_t>(std::size(wait_semaphores));

      auto const signal_semaphore_count =
        static_cast<uint32_t>(std::size(signal_semaphores));

      auto const command_buffers_ptr =
        &swp_ctx_.command_buffers_.get()[current_index];

      auto info                 = VkSubmitInfo();
      info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      info.waitSemaphoreCount   = wait_semaphore_count;
      info.pWaitSemaphores      = std::data(wait_semaphores);
      info.pWaitDstStageMask    = std::data(wait_stages);
      info.commandBufferCount   = 1;
      info.pCommandBuffers      = command_buffers_ptr;
      info.signalSemaphoreCount = signal_semaphore_count;
      info.pSignalSemaphores    = std::data(signal_semaphores);
      return info;
    }();

    frame_in_flight_fence.reset();

    auto [graphics_queue, present_queue] = ctx_.device_.get_queues();
    graphics_queue.submit(submit_info, frame_in_flight_fence);

    auto const present_info = [&signal_semaphores, &swapchains, &current_index]
    {
      auto const wait_semaphore_count =
        static_cast<uint32_t>(std::size(signal_semaphores));

      auto const swapchain_count =
        static_cast<uint32_t>(std::size(swapchains));

      auto info               = VkPresentInfoKHR();
      info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
      info.waitSemaphoreCount = wait_semaphore_count;
      info.pWaitSemaphores    = std::data(signal_semaphores);
      info.swapchainCount     = swapchain_count;
      info.pSwapchains        = std::data(swapchains);
      info.pImageIndices      = &current_index;
      info.pResults           = nullptr;
      return info;
    }();

    auto       present_result       = VK_ERROR_UNKNOWN;
    auto const check_present_result = [&present_result](auto const result)
    {
      present_result = result;
    };

    present_queue.present(present_info, check_present_result).wait_idle();

    auto const resized = ctx_.window_.framebuffer_resized();
    auto const change_swapchain =
      (present_result == VK_ERROR_OUT_OF_DATE_KHR) ||
      (present_result == VK_SUBOPTIMAL_KHR);

    if (change_swapchain || resized)
    {
      ctx_.window_.set_framebuffer_resized(false);
      recreate_swapchain();
      continue;
    }

    MVK_VERIFY(VK_SUCCESS == present_result);

    current_frame = (current_frame + 1) % max_frames_in_flight;
  }

  ctx_.device_.wait_idle();
}

} // namespace mvk
