#include "buffer_manager.hpp"

#include "detail/misc.hpp"
#include "utility/verify.hpp"

namespace mvk
{
namespace detail
{
static constexpr VkBufferUsageFlags
get_usage(buffer_manager::type type) noexcept;

} // namespace detail

buffer_manager::buffer_manager(types::device const device,
                               types::physical_device const physical_device,
                               types::command_pool const command_pool,
                               types::queue const graphics_queue,
                               type const type,
                               types::device_size const default_size)
    : device_(device), physical_device_(physical_device),
      command_pool_(command_pool), graphics_queue_(graphics_queue),
      type_(type)
{
  create_new_buffers_and_memories(default_size);
}

// TODO(samuel): swap buffers on every call, so there is no need to reallocate
// as much with the downside of needed to call allocate on every element using
// a buffer each time
[[nodiscard]] buffer_manager::allocation
buffer_manager::map(utility::slice<std::byte> const src)
{
  auto const size = std::size(src);
  auto const next_offset = current_offset() + size;

  auto const buffer_size = aligned_size_;

  if (buffer_size < next_offset)
  {
    // if this goes through, it invalidates previous info
    add_current_buffers_and_memories_to_garbage();
    create_new_buffers_and_memories(next_offset * 2);
  }

  auto const offset = current_offset();
  // TODO(samuel): stage creates a new staging buffer and memory on every
  // call
  detail::stage(device_, physical_device_, graphics_queue_, command_pool_,
                types::decay(current_buffer()), src, offset);
  return {current_buffer(), update_current_offset(size)};
}

void
buffer_manager::create_new_buffers_and_memories(
    types::device_size const size) noexcept
{
  auto const vertex_buffer_create_info = [this, size]
  {
    auto info = VkBufferCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = detail::get_usage(type_);
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    return info;
  }();

  auto const create_buffer = [this, &vertex_buffer_create_info]
  {
    return types::unique_buffer::create(types::get(device_),
                                        vertex_buffer_create_info);
  };

  std::generate(std::begin(buffers_), std::end(buffers_), create_buffer);
  std::fill(std::begin(offsets_), std::end(offsets_), 0);

  auto const requirements =
      detail::query<vkGetBufferMemoryRequirements>::with(
          types::parent(current_buffer()), types::get(current_buffer()));

  aligned_size_ = [requirements]
  {
    auto const required_size = requirements.size;
    auto const required_alignment = requirements.alignment;

    auto const aligned_module = required_size % required_alignment;

    if (aligned_module == 0)
    {
      return required_size;
    }

    return required_size + required_alignment - aligned_module;
  }();

  auto const memory_type_index = detail::find_memory_type(
      types::get(physical_device_), requirements.memoryTypeBits,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  MVK_VERIFY(memory_type_index.has_value());

  auto allocate_info = VkMemoryAllocateInfo();
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.allocationSize = std::size(buffers_) * aligned_size_;
  allocate_info.memoryTypeIndex = memory_type_index.value();
  buffers_memory_ =
      types::unique_device_memory::create(types::get(device_), allocate_info);

  for (size_t i = 0; i < std::size(buffers_); ++i)
  {
    auto const current_size = i * aligned_size_;
    vkBindBufferMemory(types::parent(buffers_[i]), types::get(buffers_[i]),
                       types::get(buffers_memory_), current_size);
  }
}

void
buffer_manager::add_current_buffers_and_memories_to_garbage()
{
  auto const buffer_to_garbage = [this](auto & buffer)
  {
    auto & buffers = current_garbage_buffers();
    buffers.push_back(std::move(buffer));
  };

  std::for_each(std::begin(buffers_), std::end(buffers_), buffer_to_garbage);
  current_garbage_memories().push_back(std::move(buffers_memory_));
}

void
buffer_manager::clear_garbage()
{
  current_garbage_index_ =
      (current_garbage_index_ + 1) % std::size(garbage_buffers_);
  current_garbage_buffers().clear();
  current_garbage_memories().clear();
}

void
buffer_manager::next_frame()
{
  current_buffer_index_ = (current_buffer_index_ + 1) % std::size(buffers_);
  offsets_[current_buffer_index_] = 0;
}

namespace detail
{
static constexpr VkBufferUsageFlags
get_usage(buffer_manager::type const type) noexcept
{
  switch (type)
  {
  case buffer_manager::type::index:
    return VK_BUFFER_USAGE_TRANSFER_DST_BIT |
           VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

  case buffer_manager::type::vertex:
    return VK_BUFFER_USAGE_TRANSFER_DST_BIT |
           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

  case buffer_manager::type::none:
    MVK_VERIFY_NOT_REACHED();
    return {};
  }
}

} // namespace detail

shader_stage_builder &
shader_stage_builder::add_stage(types::unique_shader_module shader_module,
                                VkShaderStageFlagBits const stage) noexcept
{
  auto info = VkPipelineShaderStageCreateInfo();
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  info.stage = stage;
  info.module = types::get(shader_module);
  info.pName = "main";

  shader_modules_.push_back(std::move(shader_module));
  stages_.push_back(info);

  return *this;
}

} // namespace mvk
