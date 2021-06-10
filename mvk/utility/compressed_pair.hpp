#ifndef MVK_UTILITY_COMPRESSED_PAIR_HPP_INCLUDED
#define MVK_UTILITY_COMPRESSED_PAIR_HPP_INCLUDED

#include "utility/concepts.hpp"

#include <type_traits>

// Super naive compressed_pair implementation, let's hope it has no bugs!
namespace mvk::utility
{

template <size_t Index, typename T, bool = std::is_empty_v<T>>
class compressed_pair_element : private T
{
        using type = compressed_pair_element;

public:
        using value_type      = T;
        using reference       = T &;
        using const_reference = T const &;

        constexpr compressed_pair_element() noexcept = default;

        template <typename U>
        requires same_as<std::decay_t<U>, value_type>
        constexpr explicit compressed_pair_element(U && value) noexcept : T(std::forward<U>(value))
        {
        }

        [[nodiscard]] constexpr reference
        get() noexcept
        {
                return static_cast<T &>(*this);
        }

        [[nodiscard]] constexpr const_reference
        get() const noexcept
        {
                return static_cast<T const &>(*this);
        }
};

template <size_t Index, typename T>
class compressed_pair_element<Index, T, false>
{
        using type = compressed_pair_element;

public:
        using value_type      = T;
        using reference       = T &;
        using const_reference = T const &;

        constexpr compressed_pair_element() noexcept = default;

        template <typename U>
        requires same_as<std::decay_t<U>, value_type>
        constexpr explicit compressed_pair_element(U && value) noexcept : value_(std::forward<U>(value))
        {
        }

        [[nodiscard]] constexpr reference
        get() noexcept
        {
                return value_;
        }

        [[nodiscard]] constexpr const_reference
        get() const noexcept
        {
                return value_;
        }

private:
        value_type value_;
};

template <typename First, typename Second>
class compressed_pair : private compressed_pair_element<0, First>, private compressed_pair_element<1, Second>
{
        using first_element_type  = compressed_pair_element<0, First>;
        using second_element_type = compressed_pair_element<1, Second>;
        static_assert(!std::is_same_v<First, Second>, "same types");

public:
        using first_type  = typename first_element_type::value_type;
        using second_type = typename second_element_type::value_type;

        constexpr compressed_pair() noexcept = default;

        template <typename U1, typename U2>
        constexpr compressed_pair(U1 && first, U2 && second) noexcept
                : first_element_type(std::forward<U1>(first)),
                  second_element_type(std::forward<U2>(second))
        {
        }

        struct first_tag
        {
        };

        template <typename U1>
        constexpr compressed_pair([[maybe_unused]] first_tag tag, U1 && first) noexcept : first_element_type(std::forward<U1>(first))
        {
        }

        struct second_tag
        {
        };

        template <typename U2>
        constexpr compressed_pair([[maybe_unused]] second_tag tag, U2 && second) noexcept : second_element_type(std::forward<U2>(second))
        {
        }

        [[nodiscard]] constexpr typename first_element_type::const_reference
        first() const noexcept
        {
                return static_cast<first_element_type const &>(*this).get();
        }

        [[nodiscard]] constexpr typename second_element_type::const_reference
        second() const noexcept
        {
                return static_cast<second_element_type const &>(*this).get();
        }

        [[nodiscard]] constexpr typename first_element_type::reference
        first() noexcept
        {
                return static_cast<first_element_type &>(*this).get();
        }

        [[nodiscard]] constexpr typename second_element_type::reference
        second() noexcept
        {
                return static_cast<second_element_type &>(*this).get();
        }

        void
        swap(compressed_pair & other) noexcept
        {
                std::swap(first(), other.first());
                std::swap(second(), other.second());
        }
};

} // namespace mvk::utility

#endif
