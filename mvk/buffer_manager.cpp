#include "buffer_manager.hpp"

#include "utility/verify.hpp"

namespace mvk
{

namespace detail
{

static constexpr VkBufferUsageFlags
get_usage(buffer_type type);

} // namespace detail

buffer_manager::buffer_manager(types::device * const device, types::command_pool * const command_pool, buffer_type const type, VkDeviceSize const default_size)
    : device_(device), command_pool_(command_pool), type_(type)
{
    create_new_buffers_and_memories(default_size);
}

// TODO(samuel): swap buffers on every call, so there is no need to reallocate
// as much with the downside of needed to call allocate on every element using
// a buffer each time
[[nodiscard]] buffer_manager::allocation
buffer_manager::map(utility::slice<std::byte> const data_source)
{
    auto const size        = std::size(data_source);
    auto const next_offset = current_offset() + size;
    auto const buffer_size = std::size(current_buffer());

    if (buffer_size < next_offset)
    {
        // if this goes through, it invalidates previous info
        add_current_buffers_and_memories_to_garbage();
        create_new_buffers_and_memories(next_offset * 2);
    }

    auto const offset = current_offset();
    // TODO(samuel): stage creates a new staging buffer and memory on every call
    current_buffer().stage(*device_, *command_pool_, data_source, offset);
    return {current_buffer(), update_current_offset(size)};
}

void
buffer_manager::create_new_buffers_and_memories(VkDeviceSize const size)
{
    auto const vertex_buffer_create_info = [this, size]
    {
        auto info        = VkBufferCreateInfo();
        info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size        = size;
        info.usage       = detail::get_usage(type_);
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        return info;
    }();

    auto const create_buffer = [this, &vertex_buffer_create_info]
    {
        return types::buffer(device_->get(), vertex_buffer_create_info);
    };

    std::generate(std::begin(buffers_), std::end(buffers_), create_buffer);
    std::fill(std::begin(offsets_), std::end(offsets_), 0);

    auto const requirements = current_buffer().memory_requirements();

    aligned_size_ = [requirements]
    {
        auto const required_size      = requirements.size;
        auto const required_alignment = requirements.alignment;

        auto const aligned_module = required_size % required_alignment;

        if (aligned_module == 0)
        {
            return required_size;
        }

        return required_size + required_alignment - aligned_module;
    }();

    auto const physical_device   = device_->physical_device();
    auto const memory_type_index = types::detail::find_memory_type(physical_device, requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    MVK_VERIFY(memory_type_index.has_value());

    auto allocate_info            = VkMemoryAllocateInfo();
    allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize  = std::size(buffers_) * aligned_size_;
    allocate_info.memoryTypeIndex = memory_type_index.value();

    buffers_memory_ = types::device_memory(device_->get(), allocate_info);

    for (size_t i = 0; i < std::size(buffers_); ++i)
    {
        auto const current_size = i * aligned_size_;
        buffers_memory_.bind(buffers_[i], current_size);
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
    current_garbage_index_ = (current_garbage_index_ + 1) % std::size(buffers_);
    current_garbage_buffers().clear();
    current_garbage_memories().clear();
}

void
buffer_manager::next_frame()
{
    current_buffer_index_ = (current_buffer_index_ + 1) % std::size(buffers_);
}

namespace detail
{

static constexpr VkBufferUsageFlags
get_usage(buffer_type const type)
{
    switch (type)
    {
        case buffer_type::index:
            return VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        case buffer_type::vertex:
            return VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        case buffer_type::ubo:
            return VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        case buffer_type::none:
            MVK_VERIFY_NOT_REACHED();
    }
}

} // namespace detail

} // namespace mvk
