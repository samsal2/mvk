#ifndef MVK_UTILITY_Slice_HPP
#define MVK_UTILITY_Slice_HPP

#include "utility/concepts.hpp"
#include "utility/types.hpp"
#include "utility/verify.hpp"

#include <limits>
#include <type_traits>
namespace mvk::utility
{
  static constexpr size_t DynamicExtent = std::numeric_limits<size_t>::max();

  namespace detail
  {
    template <typename T, size_t Size>
    struct SliceStorage
    {
      constexpr SliceStorage() noexcept = default;

      explicit SliceStorage(T * const Ptr, [[maybe_unused]] size_t const Count) noexcept : Data(Ptr) {}

      [[nodiscard]] constexpr size_t size() const noexcept
      {
        return Extent;
      }

      T *                     Data   = nullptr;
      static constexpr size_t Extent = Size;
    };

    template <typename T>
    struct SliceStorage<T, DynamicExtent>
    {
      constexpr SliceStorage() noexcept = default;

      explicit SliceStorage(T * const Ptr, size_t const Size) noexcept : Data(Ptr), Extent(Size) {}

      [[nodiscard]] constexpr size_t size() const noexcept
      {
        return Extent;
      }

      T *    Data   = nullptr;
      size_t Extent = 0;
    };

    template <typename It, typename Elem, typename Val = detail::ValueTypeFromIt<It>>
    concept ValidSliceIt = requires
    {
      requires RandomAccess<It>;
      requires ConvertibleAsArrayTo<Val, Elem>;
    };

  }  // namespace detail

  template <typename T, size_t Extent = DynamicExtent>
  class Slice
  {
  public:
    using value_type             = std::remove_cv_t<T>;
    using element_type           = T;
    using size_type              = size_t;
    using difference_type        = ptrdiff_t;
    using pointer                = element_type *;
    using const_pointer          = element_type const *;
    using reference              = element_type &;
    using const_reference        = element_type const &;
    using iterator               = pointer;
    using const_iterator         = const_pointer;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr auto extent = Extent;

    constexpr Slice() noexcept = default;

    template <typename It>
    requires detail::ValidSliceIt<It, element_type>
    constexpr explicit(Extent != DynamicExtent) Slice(It Begin, size_type Count) noexcept
      : Storage(detail::unwrapIt(Begin), Count)
    {
      MVK_VERIFY(Extent == DynamicExtent || Extent == size());
    }

    template <typename It>
    requires detail::ValidSliceIt<It, element_type>
    constexpr explicit(Extent != DynamicExtent) Slice(It begin, It end) noexcept
      : Storage(detail::unwrapIt(begin), std::distance(begin, end))
    {
      MVK_VERIFY(Extent == DynamicExtent || Extent == size());
    }

    /*
    template <size_t N>
    constexpr explicit Slice(element_type (&array)[N])
      : Storage(std::begin(array), N)
    {
      MVK_VERIFY(Extent == DynamicExtent || Extent == size());
    }
    */

    template <typename U, size_t N>
    requires ConvertibleAsArrayTo<U, element_type>
    constexpr Slice(std::array<U, N> & Arr) noexcept : Storage(std::data(Arr), std::size(Arr))
    {
      MVK_VERIFY(Extent == DynamicExtent || Extent == size());
    }

    template <typename U, size_t N>
    requires ConvertibleAsArrayTo<U, element_type>
    constexpr Slice(std::array<U, N> const & Arr) noexcept : Storage(std::data(Arr), std::size(Arr))
    {
      MVK_VERIFY(Extent == DynamicExtent || Extent == size());
    }

    template <typename Container>
    requires WithDataAndSize<Container> && CompatibleWithElement<Container, element_type>
    constexpr Slice(Container & Contr) noexcept : Storage(std::data(Contr), std::size(Contr))
    {
      MVK_VERIFY(Extent == DynamicExtent || Extent == size());
    }

    template <typename U, size_t N>
    requires ConvertibleAsArrayTo<U, element_type>
    constexpr explicit(Extent != DynamicExtent && N == DynamicExtent) Slice(Slice<U, N> Other) noexcept
      : Storage(std::data(Other), std::size(Other))
    {
      MVK_VERIFY(Extent == DynamicExtent || Extent == size());
    }

    template <typename U>
    requires ConvertibleAsArrayTo<U, element_type>
    constexpr Slice(U & value) noexcept : Storage(&value, 1) {}

    [[nodiscard]] constexpr bool empty() const noexcept
    {
      return size() == 0;
    }

    [[nodiscard]] constexpr size_type size() const noexcept
    {
      return std::size(Storage);
    }

    [[nodiscard]] constexpr pointer data() const noexcept
    {
      return Storage.Data;
    }

    [[nodiscard]] constexpr iterator begin() const noexcept
    {
      return &data()[0];
    }

    [[nodiscard]] constexpr iterator end() const noexcept
    {
      return &data()[size()];
    }

    [[nodiscard]] constexpr reference operator[](size_t const Idx) const noexcept
    {
      MVK_VERIFY(Idx < size());
      return data()[Idx];
    }

    [[nodiscard]] constexpr Slice last(size_type const Cnt) const noexcept
    {
      return { &operator[](size() - Cnt), Cnt };
    }

    [[nodiscard]] constexpr Slice first(size_type const Cnt) const noexcept
    {
      return { &operator[](0), Cnt };
    }

    [[nodiscard]] constexpr Slice<element_type> subSlice(size_type const Off,
                                                         size_type const Cnt = DynamicExtent) const noexcept
    {
      auto const NewData = &operator[](Off);

      if (Cnt == DynamicExtent)
      {
        return { NewData, size() - Off };
      }

      MVK_VERIFY((Off + Cnt) < size());
      return { NewData, Cnt };
    }

  private:
    detail::SliceStorage<T, Extent> Storage;
  };

  template <typename T>
  Slice(T *, size_t) -> Slice<T>;

  template <typename T, size_t S>
  Slice(std::array<T, S> const &) -> Slice<T const, S>;

  template <typename T, size_t S>
  Slice(std::array<T, S> const &) -> Slice<T, S>;

  template <typename T>
  Slice(T &) -> Slice<detail::ValueTypeFromData<T>>;

  template <typename T, size_t E>
  [[nodiscard]] constexpr Slice<std::byte const, E * sizeof(T)> as_bytes(Slice<T const, E> const Src) noexcept
  {
    return { force_cast_to_byte(std::data(Src)), E * sizeof(T) };
  }

  template <typename T>
  [[nodiscard]] constexpr Slice<std::byte const> as_bytes(Slice<T const> const Src) noexcept
  {
    auto const size = std::size(Src);
    return { forceCastToByte(std::data(Src)), size * sizeof(T) };
  }

  template <typename Container>
  requires WithDataAndSize<Container>
  [[nodiscard]] constexpr Slice<std::byte const> as_bytes(Container const & Src) noexcept
  {
    return as_bytes(Slice(Src));
  }

  template <typename T>
  requires Trivial<T>
  [[nodiscard]] constexpr Slice<std::byte const> as_bytes(T const & Src) noexcept
  {
    auto const size = sizeof(T);
    return { forceCastToByte(&Src), size };
  }

}  // namespace mvk::utility

#endif
