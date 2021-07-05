#ifndef MVK_UTILITY_OUT_HPP_INCLUDED
#define MVK_UTILITY_OUT_HPP_INCLUDED

#include "utility/types.hpp"

#include <type_traits>

namespace mvk
{
  template <typename T>
  class non_null
  {
  public:
    using value_type      = T;
    using pointer         = T *;
    using reference       = T &;
    using const_reference = T const &;

    constexpr non_null(pointer ptr) noexcept : ptr_(ptr) {}

    constexpr non_null(std::nullptr_t) noexcept = delete;
    non_null & operator=(std::nullptr_t) noexcept = delete;

    constexpr reference get() const noexcept
    {
      return *ptr_;
    }

    constexpr operator const_reference() const
    {
      return get();
    }

    constexpr pointer operator->() const
    {
      return &get();
    }

    constexpr reference operator*() const
    {
      return get();
    }

    constexpr operator pointer() const
    {
      return &get();
    }

    non_null & operator++() noexcept               = delete;
    non_null & operator--() noexcept               = delete;
    non_null   operator++(int) noexcept            = delete;
    non_null   operator--(int) noexcept            = delete;
    non_null & operator+=(std::ptrdiff_t) noexcept = delete;
    non_null & operator-=(std::ptrdiff_t) noexcept = delete;
    void       operator[](std::ptrdiff_t) const    = delete;

  private:
    pointer ptr_;
  };

  template <typename T>
  class nullable
  {
  public:
    using value_type      = T;
    using pointer         = T *;
    using reference       = T &;
    using const_reference = T const &;

    constexpr nullable(pointer ptr) noexcept : ptr_(ptr) {}

    constexpr reference get() const noexcept
    {
      return *ptr_;
    }

    constexpr operator const_reference() const
    {
      return get();
    }

    constexpr pointer operator->() const
    {
      return &get();
    }
    constexpr reference operator*() const
    {
      return get();
    }

    constexpr operator bool() const
    {
      return ptr_ != nullptr;
    }

    nullable & operator++() noexcept               = delete;
    nullable & operator--() noexcept               = delete;
    nullable   operator++(int) noexcept            = delete;
    nullable   operator--(int) noexcept            = delete;
    nullable & operator+=(std::ptrdiff_t) noexcept = delete;
    nullable & operator-=(std::ptrdiff_t) noexcept = delete;
    void       operator[](std::ptrdiff_t) const    = delete;

  private:
    pointer ptr_;
  };

  template <typename T>
  class out;

  template <typename T>
  class in_out : public non_null<T>
  {
  public:
    static_assert(!std::is_const<T>::value, "T cannot be const");

    using non_null = non_null<T>;
    using non_null::non_null;

    constexpr in_out(out<T> ptr) noexcept : non_null(ptr) {}
  };

  template <typename T>
  class out : public non_null<T>
  {
  public:
    static_assert(!std::is_const<T>::value, "T cannot be const");

    using non_null = non_null<T>;
    using non_null::non_null;

    constexpr out(in_out<T> ptr) noexcept : non_null(ptr) {}
  };
}  // namespace mvk

#endif
