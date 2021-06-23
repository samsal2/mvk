#ifndef MVK_WRAPPER_OBJECT_ALLOCATE_HPP_INCLUDED
#define MVK_WRAPPER_OBJECT_ALLOCATE_HPP_INCLUDED

#include "wrapper/fwd.hpp"

#include <vector>

namespace mvk::wrapper
{
namespace creator
{
struct object_allocate
{
};

} // namespace creator

template <auto Call, typename Wrapper>
class object_allocate;

template <typename... Args>
constexpr auto
creator_selector([[maybe_unused]] creator::object_allocate option)
{
  using wrapper = any_wrapper<Args...>;

  constexpr auto call = select<options::creator_call>(Args{}...);
  static_assert(!utility::is_none(call), "Expected creator_call option");

  return detail::select<object_allocate<call, wrapper>>{};
}

template <auto Call, typename Wrapper>
class object_allocate
{
  using wrapper_type = Wrapper;
  static constexpr auto create_call = Call;

public:
  template <typename Parent, typename Info>
  [[nodiscard]] static std::vector<wrapper_type>
  allocate(Parent const parent, Info const & info) noexcept
  {
    auto const size = detail::size_from_info(info);

    using handle_type = typename wrapper_type::handle_type;
    auto handles = std::vector<handle_type>(size);
    create_call(parent, &info, std::data(handles));

    auto wrappers = std::vector<wrapper_type>();
    wrappers.reserve(size);

    auto const pool = detail::pool_from_info(info);

    for (auto const handle : handles)
    {
      wrappers.emplace_back(handle, parent, pool);
    }

    return wrappers;
  }
};

} // namespace mvk::wrapper

#endif
