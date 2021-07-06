#ifndef MVK_UTILITY_BADGE_HPP_INCLUDED
#define MVK_UTILITY_BADGE_HPP_INCLUDED

namespace Mvk::Utility {

template <typename Owner> class Badge {
  friend Owner;
  constexpr Badge() noexcept = default;
};

} // namespace Mvk::Utility

#endif
