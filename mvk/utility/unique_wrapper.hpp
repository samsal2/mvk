#ifndef MVK_UTILITY_UNIQUE_WRAPPER_HPP_INCLUDED
#define MVK_UTILITY_UNIQUE_WRAPPER_HPP_INCLUDED

#include "utility/compressed_pair.hpp"

// unique_ptr must take a pointer, unique_wrapper
// I don't need that indirection of inderection

namespace mvk::utility
{

template <typename T, typename Deleter>
class unique_wrapper
{
public:
        using value_type      = T;
        using reference       = T &;
        using const_reference = T const &;
        using deleter_type    = Deleter;

        constexpr unique_wrapper() noexcept = default;

        unique_wrapper(unique_wrapper const &) noexcept = delete;

        unique_wrapper(unique_wrapper && other) noexcept
        {
                swap(other);
        }

        unique_wrapper &
        operator=(unique_wrapper const &) noexcept = delete;

        unique_wrapper &
        operator=(unique_wrapper && other) noexcept
        {
                swap(other);
                return *this;
        }

        ~unique_wrapper() noexcept
        {
                destroy();
        }

        template <typename U1, typename U2>
        constexpr unique_wrapper(U1 && container, U2 && deleter) noexcept : container_(std::forward<U1>(container), std::forward<U2>(deleter))
        {
        }

        [[nodiscard]] constexpr value_type const &
        get() const noexcept
        {
                return container_.first();
        }

        [[nodiscard]] constexpr value_type &
        get() noexcept
        {
                return container_.first();
        }

        [[nodiscard]] constexpr deleter_type const &
        deleter() const noexcept
        {
                return container_.second();
        }

        [[nodiscard]] constexpr deleter_type &
        deleter() noexcept
        {
                return container_.first();
        }

        void
        swap(unique_wrapper & other) noexcept
        {
                container_.swap(other.container_);
        }

        void
        destroy() const noexcept
        {
                deleter()(get());
        }

private:
        compressed_pair<T, Deleter> container_;
};

} // namespace mvk::utility

#endif
