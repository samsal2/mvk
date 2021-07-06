#ifndef MVK_UTILITY_OUT_HPP_INCLUDED
#define MVK_UTILITY_OUT_HPP_INCLUDED

#include "Utility/Concepts.hpp"
#include "Utility/Types.hpp"

#include <cstddef>
#include <type_traits>

namespace Mvk {
template <typename T> class NonNull;

template <typename T> class Out;

template <typename T> class InOut;

template <typename T> class In;

template <typename T> class NonNull {
public:
  constexpr NonNull(T *Val) noexcept : Ptr(Val) {}

  constexpr NonNull(std::nullptr_t) noexcept = delete;
  NonNull operator=(std::nullptr_t) noexcept = delete;
  NonNull &operator++() = delete;
  NonNull &operator--() = delete;
  NonNull operator++(int) = delete;
  NonNull operator--(int) = delete;
  NonNull &operator+=(std::ptrdiff_t) = delete;
  NonNull &operator-=(std::ptrdiff_t) = delete;
  void operator[](std::ptrdiff_t) const = delete;

  constexpr T const &operator*() const noexcept { return *Ptr; }

  constexpr T &operator*() noexcept { return *Ptr; }

  constexpr T const *operator->() const noexcept { return Ptr; }

  constexpr T *operator->() noexcept { return Ptr; }

private:
  friend NonNull<T const>;

  T *Ptr;
};

template <typename T> class NonNull<T const> {
public:
  constexpr NonNull(T const *Val) noexcept : Ptr(Val) {}
  constexpr NonNull(NonNull<T> Other) noexcept : Ptr(Other.Ptr) {}

  constexpr NonNull(std::nullptr_t) noexcept = delete;
  NonNull operator=(std::nullptr_t) noexcept = delete;
  NonNull &operator++() = delete;
  NonNull &operator--() = delete;
  NonNull operator++(int) = delete;
  NonNull operator--(int) = delete;
  NonNull &operator+=(std::ptrdiff_t) = delete;
  NonNull &operator-=(std::ptrdiff_t) = delete;
  void operator[](std::ptrdiff_t) const = delete;

  constexpr T const &operator*() const noexcept { return *Ptr; }

  constexpr T const *operator->() const noexcept { return Ptr; }

private:
  T const *Ptr;
};

template <typename T> NonNull(T *) -> NonNull<T>;

// Doesnt need the reference to be initialized
template <typename T> class Out : public NonNull<T> {
public:
  static_assert(std::is_default_constructible_v<T>,
                "Out requires to be default constructible");

  using NonNull = NonNull<T>;
  using NonNull::NonNull;
};

template <typename T> Out(T *) -> Out<T>;

// Expects the reference to be constructed
template <typename T> class InOut : public NonNull<T> {
public:
  using NonNull = NonNull<T>;
  using NonNull::NonNull;

  constexpr InOut(Out<T> Out) noexcept : NonNull(Out) {}
};

template <typename T> InOut(T *) -> InOut<T>;

template <typename T> class In : public NonNull<T const> {
public:
  using NonNull = NonNull<T const>;
  using NonNull::NonNull;

  constexpr In(InOut<T> InOut) noexcept : NonNull(InOut) {}
};

template <typename T> In(T *) -> In<T>;

} // namespace Mvk

#endif
