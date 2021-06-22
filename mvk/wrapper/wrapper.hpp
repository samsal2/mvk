#ifndef MVK_WRAPPER_WRAPPER_HPP_INCLUDED
#define MVK_WRAPPER_WRAPPER_HPP_INCLUDED

#include "wrapper/any_wrapper.hpp"
#include "wrapper/created.hpp"
#include "wrapper/fwd.hpp"
#include "wrapper/handle_only.hpp"
#include "wrapper/object_destroy.hpp"
#include "wrapper/owner_destroy.hpp"
#include "wrapper/unique.hpp"

namespace mvk::wrapper
{
template <typename... Args>
constexpr auto
decay_wrapper([[maybe_unused]] any_wrapper<Args...> const & wrapper) noexcept
{
  using handle = decltype(select<options::handle>(Args{}...));
  return any_wrapper<options::storage<storage::handle_only>,
                     options::handle<handle>>{};
}

template <typename Wrapper>
using decay_wrapper_t = decltype(decay_wrapper(std::declval<Wrapper>()));

}; // namespace mvk::wrapper

#endif
