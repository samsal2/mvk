#ifndef MVK_UTILITY_SLICE_HPP
#define MVK_UTILITY_SLICE_HPP

#include "utility/concepts.hpp"
#include "utility/verify.hpp"

#include <limits>
#include <type_traits>
namespace mvk::utility
{
static constexpr size_t dynamic_extent = std::numeric_limits<size_t>::max();

namespace detail
{
template <typename T, size_t Extent>
struct slice_storage
{
  constexpr slice_storage() noexcept = default;

  explicit slice_storage(T * const data,
                         [[maybe_unused]] size_t const size) noexcept
      : data_(data)
  {
  }

  [[nodiscard]] constexpr size_t
  size() const noexcept
  {
    return extent;
  }

  T * data_ = nullptr;
  static constexpr size_t extent = Extent;
};

template <typename T>
struct slice_storage<T, dynamic_extent>
{
  constexpr slice_storage() noexcept = default;

  explicit slice_storage(T * const data, size_t const size) noexcept
      : data_(data), size_(size)
  {
  }

  [[nodiscard]] constexpr size_t
  size() const noexcept
  {
    return size_;
  }

  T * data_ = nullptr;
  size_t size_ = 0;
};

template <typename Iterator, typename Element,
          typename Value = detail::value_type_from_iterator_t<Iterator>>
concept valid_slice_iterator = requires
{
  requires random_access<Iterator>;
  requires convertible_as_array_to<Value, Element>;
};

} // namespace detail

template <typename T, size_t Extent = dynamic_extent>
class slice
{
public:
  using value_type = std::remove_cv_t<T>;
  using element_type = T;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using pointer = element_type *;
  using const_pointer = element_type const *;
  using reference = element_type &;
  using const_reference = element_type const &;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  static constexpr auto extent = Extent;

  constexpr slice() noexcept = default;

  template <typename Iterator>
  requires detail::valid_slice_iterator<Iterator, element_type>
  constexpr explicit(extent != dynamic_extent)
      slice(Iterator begin, size_type count) noexcept
      : storage_(detail::unwrap_iterator(begin), count)
  {
    MVK_VERIFY(Extent == dynamic_extent || Extent == size());
  }

  template <typename Iterator>
  requires detail::valid_slice_iterator<Iterator, element_type>
  constexpr explicit(extent != dynamic_extent)
      slice(Iterator begin, Iterator end) noexcept
      : storage_(detail::unwrap_iterator(begin), std::distance(begin, end))
  {
    MVK_VERIFY(extent == dynamic_extent || extent == size());
  }

  /*
  template <size_t N>
  constexpr explicit slice(element_type (&array)[N])
    : storage_(std::begin(array), N)
  {
    MVK_VERIFY(extent == dynamic_extent || extent == size());
  }
  */

  template <typename U, size_t N>
  requires convertible_as_array_to<U, element_type>
  constexpr slice(std::array<U, N> & array) noexcept
      : storage_(std::data(array), std::size(array))
  {
    MVK_VERIFY(extent == dynamic_extent || extent == size());
  }

  template <typename U, size_t N>
  requires convertible_as_array_to<U, element_type>
  constexpr slice(std::array<U, N> const & array) noexcept
      : storage_(std::data(array), std::size(array))
  {
    MVK_VERIFY(extent == dynamic_extent || extent == size());
  }

  template <typename Container>
  requires with_data_and_size<Container> &&
      compatible_with_element<Container, element_type>
  constexpr slice(Container & container) noexcept
      : storage_(std::data(container), std::size(container))
  {
    MVK_VERIFY(extent == dynamic_extent || extent == size());
  }

  template <typename U, size_t N>
  requires convertible_as_array_to<U, element_type>
  constexpr explicit(extent != dynamic_extent && N == dynamic_extent)
      slice(slice<U, N> other) noexcept
      : storage_(std::data(other), std::size(other))
  {
    MVK_VERIFY(extent == dynamic_extent || extent == size());
  }

  template <typename U>
  requires convertible_as_array_to<U, element_type>
  constexpr slice(U & value) noexcept : storage_(&value, 1)
  {
  }

  [[nodiscard]] constexpr bool
  empty() const noexcept
  {
    return size() == 0;
  }

  [[nodiscard]] constexpr size_type
  size() const noexcept
  {
    return std::size(storage_);
  }

  [[nodiscard]] constexpr pointer
  data() const noexcept
  {
    return storage_.data_;
  }

  [[nodiscard]] constexpr iterator
  begin() const noexcept
  {
    return &data()[0];
  }

  [[nodiscard]] constexpr iterator
  end() const noexcept
  {
    return &data()[size()];
  }

  [[nodiscard]] constexpr reference
  operator[](size_t const index) const noexcept
  {
    MVK_VERIFY(index < size());
    return data()[index];
  }

  [[nodiscard]] constexpr slice
  last(size_type const count) const noexcept
  {
    return {&operator[](size() - count), count};
  }

  [[nodiscard]] constexpr slice
  first(size_type const count) const noexcept
  {
    return {&operator[](0), count};
  }

  [[nodiscard]] constexpr slice<element_type>
  subslice(size_type const offset,
           size_type const count = dynamic_extent) const noexcept
  {
    auto const new_data = &operator[](offset);

    if (count == dynamic_extent)
    {
      return {new_data, size() - offset};
    }

    MVK_VERIFY((offset + count) < size());
    return {new_data, count};
  }

private:
  detail::slice_storage<T, extent> storage_;
};

template <typename T>
slice(T *, size_t) -> slice<T>;

template <typename T, size_t S>
slice(std::array<T, S> const &) -> slice<T const, S>;

template <typename T, size_t S>
slice(std::array<T, S> const &) -> slice<T, S>;

template <typename T>
slice(T &) -> slice<detail::value_type_from_data_t<T>>;

template <typename T, size_t E>
[[nodiscard]] constexpr slice<std::byte const, E * sizeof(T)>
as_bytes(slice<T const, E> const src) noexcept
{
  return {force_cast_to_byte(std::data(src)), E * sizeof(T)};
}

template <typename T>
[[nodiscard]] constexpr slice<std::byte const>
as_bytes(slice<T const> const src) noexcept
{
  auto const size = std::size(src);
  return {force_cast_to_byte(std::data(src)), size * sizeof(T)};
}

template <typename Container>
requires with_data_and_size<Container>
[[nodiscard]] constexpr slice<std::byte const>
as_bytes(Container const & src) noexcept
{
  return as_bytes(slice(src));
}

template <typename T>
requires trivial<T>
[[nodiscard]] constexpr slice<std::byte const>
as_bytes(T const & src) noexcept
{
  auto const size = sizeof(T);
  return {force_cast_to_byte(&src), size};
}

} // namespace mvk::utility

#endif
