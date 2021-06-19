#ifndef MVK_UTILITY_DETAIL_CONDITION_HPP_INCLUDED
#define MVK_UTILITY_DETAIL_CONDITION_HPP_INCLUDED

#include <type_traits>

namespace mvk::utility::detail
{

template <typename This> struct same_as_impl
{
        template <typename Other> static constexpr auto check([[maybe_unused]] Other type) noexcept
        {
                return std::false_type{};
        }

        static constexpr auto check([[maybe_unused]] This type) noexcept
        {
                return std::true_type{};
        }
};

template <typename This> constexpr auto same_as()
{
        return same_as_impl<This>{};
}

template <template <typename> typename Tag> struct tagged_with_impl_1
{
        template <typename NotTagged> static constexpr auto check([[maybe_unused]] NotTagged type) noexcept
        {
                return std::false_type{};
        }

        template <typename T> static constexpr auto check([[maybe_unused]] Tag<T> type) noexcept
        {
                return std::true_type{};
        }
};

template <template <auto> typename Tag> struct tagged_with_impl_2
{
        template <typename NotTagged> static constexpr auto check([[maybe_unused]] NotTagged type) noexcept
        {
                return std::false_type{};
        }

        template <auto V> static constexpr auto check([[maybe_unused]] Tag<V> type) noexcept
        {
                return std::true_type{};
        }
};

template <template <typename> typename Tag> constexpr auto tagged_with()
{
        return tagged_with_impl_1<Tag>{};
}

template <template <auto> typename Tag> constexpr auto tagged_with()
{
        return tagged_with_impl_2<Tag>{};
}

} // namespace mvk::utility::detail

#endif
