#pragma once

// Namespace kinda useless here
namespace Mvk::Engine
{
#define MVK_DEFINE_DEFAULT_COPYABLE( Type )      \
  Type( Type const & Other ) noexcept = default; \
  Type & operator=( Type const & Other ) noexcept = default

#define MVK_DEFINE_NON_COPYABLE( Type )         \
  Type( Type const & Other ) noexcept = delete; \
  Type & operator=( Type const & Other ) noexcept = delete

#define MVK_DEFINE_DEFAULT_MOVABLE( Type )  \
  Type( Type && Other ) noexcept = default; \
  Type & operator=( Type && Other ) noexcept = default

#define MVK_DEFINE_NON_MOVABLE( Type )     \
  Type( Type && Other ) noexcept = delete; \
  Type & operator=( Type && Other ) noexcept = delete

}  // namespace Mvk::Engine