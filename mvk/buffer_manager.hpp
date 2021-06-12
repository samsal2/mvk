#ifndef MVK_BUFFER_MANAGER_HPP_INCLUDED
#define MVK_BUFFER_MANAGER_HPP_INCLUDED

#include "types/types.hpp"
#include "utility/slice.hpp"

#include <array>
#include <vector>

// Super inefficient buffer manager

namespace mvk
{

enum class buffer_type
{
    none,
    index,
    vertex,
    ubo,
};

class buffer_manager
{
public:
    static constexpr auto default_buffer_size  = 1024 * 1024;
    static constexpr auto dynamic_buffer_count = 2;
    static constexpr auto garbage_buffer_count = 3;

    constexpr buffer_manager() noexcept = default;

    buffer_manager(types::device * device, types::command_pool * command_pool, buffer_type type, VkDeviceSize default_size = default_buffer_size);

    struct allocation
    {
        types::buffer & buffer;
        VkDeviceSize    offset;
    };

    [[nodiscard]] allocation
    map(utility::slice<std::byte> data_source);

    void
    next_frame();

protected:
    void
    create_new_buffers_and_memories(VkDeviceSize size);

    void
    add_current_buffers_and_memories_to_garbage();

    void
    clear_garbage();

    [[nodiscard]] constexpr types::buffer &
    current_buffer() noexcept;

    [[nodiscard]] constexpr types::buffer const &
    current_buffer() const noexcept;

    [[nodiscard]] constexpr std::vector<types::buffer> &
    current_garbage_buffers() noexcept;

    [[nodiscard]] constexpr std::vector<types::buffer> const &
    current_garbage_buffers() const noexcept;

    [[nodiscard]] constexpr std::vector<types::device_memory> &
    current_garbage_memories() noexcept;

    [[nodiscard]] constexpr std::vector<types::device_memory> const &
    current_garbage_memories() const noexcept;

    [[nodiscard]] constexpr VkDeviceSize
    current_offset() const noexcept;

    [[nodiscard]] constexpr VkDeviceSize
    update_current_offset(VkDeviceSize size) noexcept;

private:
    // TODO(samuel): use shared_ptr
    types::device *       device_       = nullptr;
    types::command_pool * command_pool_ = nullptr;

    std::array<types::buffer, dynamic_buffer_count> buffers_        = {};
    std::array<VkDeviceSize, dynamic_buffer_count>  offsets_        = {};
    types::device_memory                            buffers_memory_ = {};

    std::array<std::vector<types::buffer>, garbage_buffer_count>        garbage_buffers_          = {};
    std::array<std::vector<types::device_memory>, garbage_buffer_count> garbage_buffers_memories_ = {};

    size_t current_buffer_index_  = 0;
    size_t current_garbage_index_ = 0;

    VkDeviceSize aligned_size_ = 0;
    buffer_type  type_         = buffer_type::none;
};

[[nodiscard]] constexpr types::buffer &
buffer_manager::current_buffer() noexcept
{
    return buffers_[current_buffer_index_];
}

[[nodiscard]] constexpr types::buffer const &
buffer_manager::current_buffer() const noexcept
{
    return buffers_[current_buffer_index_];
}

[[nodiscard]] constexpr std::vector<types::buffer> &
buffer_manager::current_garbage_buffers() noexcept
{
    return garbage_buffers_[current_garbage_index_];
}

[[nodiscard]] constexpr std::vector<types::buffer> const &
buffer_manager::current_garbage_buffers() const noexcept
{
    return garbage_buffers_[current_garbage_index_];
}

[[nodiscard]] constexpr std::vector<types::device_memory> &
buffer_manager::current_garbage_memories() noexcept
{
    return garbage_buffers_memories_[current_garbage_index_];
}

[[nodiscard]] constexpr std::vector<types::device_memory> const &
buffer_manager::current_garbage_memories() const noexcept
{
    return garbage_buffers_memories_[current_garbage_index_];
}

[[nodiscard]] constexpr VkDeviceSize
buffer_manager::current_offset() const noexcept
{
    return offsets_[current_buffer_index_];
}

[[nodiscard]] constexpr VkDeviceSize
buffer_manager::update_current_offset(VkDeviceSize const size) noexcept
{
    auto const past_offset = offsets_[current_buffer_index_];
    offsets_[current_buffer_index_] += size;
    return past_offset;
}

} // namespace mvk

#endif
