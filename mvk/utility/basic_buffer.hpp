#ifndef MVK_UTILITY_BASIC_BUFFER_HPP_INCLUDED
#define MVK_UTILITY_BASIC_BUFFER_HPP_INCLUDED

#include "utility/slice.hpp"
#include "utility/verify.hpp"

#include <memory>

namespace mvk::utility
{

template <typename T>
class basic_buffer
{
public:
    using value_type      = T;
    using reference       = T &;
    using const_reference = T const &;
    using pointer         = T *;
    using const_pointer   = T const *;
    using size_type       = size_t;

    constexpr basic_buffer() noexcept = default;

    explicit basic_buffer(size_type size, value_type const & value) : data_(make_unique<value_type[]>(size))
    {
        std::fill(&data_[0], &data_[0] + size, value);
    }

    [[nodiscard]] constexpr pointer
    data() noexcept
    {
        return data_.get();
    }

    [[nodiscard]] constexpr const_pointer
    data() const noexcept
    {
        return data_.get();
    }

    [[nodiscard]] constexpr size_type
    size() noexcept
    {
        return data_.get();
    }

    [[nodiscard]] constexpr size_type
    size() const noexcept
    {
        return data_.get();
    }

    [[nodiscard]] constexpr reference
    operator[](size_type const index)
    {
        MVK_VERIFY(index < size_);
        return data_[index];
    }

    [[nodiscard]] constexpr const_reference
    operator[](size_type const index) const
    {
        MVK_VERIFY(index < size_);
        return data_[index];
    }

private:
    std::unique_ptr<value_type[]> data_;
    size_type                     size_ = 0;
};

} // namespace mvk::utility

#endif
