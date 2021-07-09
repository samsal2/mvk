#pragma once

#include "Utility/Badge.hpp"
#include "Utility/Macros.hpp"

#include <memory>
#include <type_traits>

namespace Mvk::Utility
{
  template <typename T> struct Singleton
  {
    constexpr Singleton() noexcept = delete;
    MVK_DEFINE_NON_COPYABLE( Singleton );
    MVK_DEFINE_NON_MOVABLE( Singleton );
    constexpr ~Singleton() noexcept = default;

    constexpr Singleton( [[maybe_unused]] Badge<Singleton> Badge ) noexcept {}

    static T & the() noexcept
    {
      static auto The = std::make_unique<T>( Badge<Singleton>{} );
      return *The;
    }
  };

}  // namespace Mvk::Utility
