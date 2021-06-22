#ifndef MVK_BUFFER_MANAGER_HPP_INCLUDED
#define MVK_BUFFER_MANAGER_HPP_INCLUDED

#include "types/types.hpp"
#include "utility/slice.hpp"

#include <array>
#include <vector>

// Super inefficient buffer manager

namespace mvk
{
class buffer_manager
{
public:
  enum class type
  {
    none,
    vertex,
    index
  };

  static constexpr auto default_buffer_size = 1024 * 1024;
  static constexpr auto dynamic_buffer_count = 2;
  static constexpr auto garbage_buffer_count = 3;

  constexpr buffer_manager() noexcept = default;

  buffer_manager(types::device device, types::physical_device physical_device,
                 types::command_pool command_pool,
                 types::queue graphics_queue, type type,
                 types::device_size default_size = default_buffer_size);

  struct allocation
  {
    types::unique_buffer & buffer;
    types::device_size offset;
  };

  [[nodiscard]] allocation
  map(utility::slice<std::byte const> src);

  void
  next_frame();

protected:
  void
  create_new_buffers_and_memories(types::device_size size) noexcept;

  void
  add_current_buffers_and_memories_to_garbage();

  void
  clear_garbage();

  [[nodiscard]] constexpr types::unique_buffer &
  current_buffer() noexcept;

  [[nodiscard]] constexpr types::unique_buffer const &
  current_buffer() const noexcept;

  [[nodiscard]] constexpr std::vector<types::unique_buffer> &
  current_garbage_buffers() noexcept;

  [[nodiscard]] constexpr std::vector<types::unique_buffer> const &
  current_garbage_buffers() const noexcept;

  [[nodiscard]] constexpr std::vector<types::unique_device_memory> &
  current_garbage_memories() noexcept;

  [[nodiscard]] constexpr std::vector<types::unique_device_memory> const &
  current_garbage_memories() const noexcept;

  [[nodiscard]] constexpr types::device_size
  current_offset() const noexcept;

  [[nodiscard]] constexpr types::device_size
  update_current_offset(types::device_size size) noexcept;

private:
  types::device device_ = VK_NULL_HANDLE;
  types::physical_device physical_device_ = VK_NULL_HANDLE;
  types::command_pool command_pool_ = VK_NULL_HANDLE;
  types::queue graphics_queue_ = {};

  std::array<types::unique_buffer, dynamic_buffer_count> buffers_ = {};
  std::array<types::device_size, dynamic_buffer_count> offsets_ = {};
  types::unique_device_memory buffers_memory_ = {};

  template <typename T>
  using garbage_collection_array =
      std::array<std::vector<T>, garbage_buffer_count>;

  garbage_collection_array<types::unique_buffer> garbage_buffers_ = {};
  garbage_collection_array<types::unique_device_memory> garbage_memories_ =
      {};

  size_t current_buffer_index_ = 0;
  size_t current_garbage_index_ = 0;

  types::device_size aligned_size_ = 0;
  type type_ = type::none;
};

[[nodiscard]] constexpr types::unique_buffer &
buffer_manager::current_buffer() noexcept
{
  return buffers_[current_buffer_index_];
}

[[nodiscard]] constexpr types::unique_buffer const &
buffer_manager::current_buffer() const noexcept
{
  return buffers_[current_buffer_index_];
}

[[nodiscard]] constexpr std::vector<types::unique_buffer> &
buffer_manager::current_garbage_buffers() noexcept
{
  return garbage_buffers_[current_garbage_index_];
}

[[nodiscard]] constexpr std::vector<types::unique_buffer> const &
buffer_manager::current_garbage_buffers() const noexcept
{
  return garbage_buffers_[current_garbage_index_];
}

[[nodiscard]] constexpr std::vector<types::unique_device_memory> &
buffer_manager::current_garbage_memories() noexcept
{
  return garbage_memories_[current_garbage_index_];
}

[[nodiscard]] constexpr std::vector<types::unique_device_memory> const &
buffer_manager::current_garbage_memories() const noexcept
{
  return garbage_memories_[current_garbage_index_];
}

[[nodiscard]] constexpr types::device_size
buffer_manager::current_offset() const noexcept
{
  return offsets_[current_buffer_index_];
}

[[nodiscard]] constexpr types::device_size
buffer_manager::update_current_offset(types::device_size const size) noexcept
{
  auto const past_offset = offsets_[current_buffer_index_];
  offsets_[current_buffer_index_] += size;
  return past_offset;
}

class shader_stage_builder
{
public:
  shader_stage_builder &
  add_stage(types::unique_shader_module shader_module,
            VkShaderStageFlagBits stage) noexcept;

  [[nodiscard]] constexpr utility::slice<
      VkPipelineShaderStageCreateInfo const>
  stages() const noexcept;

private:
  std::vector<VkPipelineShaderStageCreateInfo> stages_;
  // Just to keep the shader modules alive for the duration of the stage
  // builder
  std::vector<types::unique_shader_module> shader_modules_;
};

[[nodiscard]] constexpr utility::slice<VkPipelineShaderStageCreateInfo const>
shader_stage_builder::stages() const noexcept
{
  return stages_;
}

} // namespace mvk

#endif
