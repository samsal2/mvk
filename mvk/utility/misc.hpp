#ifndef MVK_UTILITY_MISC_HPP_INCLUDED
#define MVK_UTILITY_MISC_HPP_INCLUDED

#include "utility/concepts.hpp"

namespace mvk::utility
{

namespace detail
{

template <typename Container>
struct data_and_size
{
  decltype(std::data(std::declval<Container &>())) data_;
  decltype(std::size(std::declval<Container &>())) size_;
};

struct data_and_size_as_bytes
{
  std::byte const * data_;
  size_t            size_;
};

} // namespace detail

template <typename Container>
requires with_data_and_size<Container>
[[nodiscard]] static constexpr detail::data_and_size<Container>
bind_data_and_size(Container & data_source) noexcept
{
  return {std::data(data_source), std::size(data_source)};
}

template <typename Container>
requires with_data_and_size<Container>
[[nodiscard]] static constexpr detail::data_and_size_as_bytes
bind_data_and_size_as_bytes(Container const & data_source) noexcept
{
  auto const [data, size] = bind_data_and_size(data_source);
  size_t const data_size  = size * sizeof(*data);
  return {force_cast_to_byte(data), data_size};
}

} // namespace mvk::utility

#endif