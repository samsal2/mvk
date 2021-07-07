#pragma once

namespace Mvk::Utility {

template <typename Owner> class Badge {
  friend Owner;
  constexpr Badge() noexcept = default;
};

} // namespace Mvk::Utility

