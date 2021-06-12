#ifndef MVK_TYPES_QUEUE_HPP_INCLUDED
#define MVK_TYPES_QUEUE_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/checkers.hpp"

namespace mvk::types
{

class fence;

class queue
{
public:
  constexpr queue() noexcept = default;

  queue(VkDevice device, uint32_t index);

  [[nodiscard]] constexpr VkQueue
  get() const noexcept;

  [[nodiscard]] constexpr uint32_t
  index() const noexcept;

  queue &
  wait_idle();

  queue &
  submit(VkSubmitInfo const & info, VkFence fence = nullptr);

  template <typename Checker = decltype(detail::default_result_checker)>
  requires detail::result_checker<Checker> queue &
  present(VkPresentInfoKHR const & info,
          Checker && check = detail::default_result_checker);

private:
  VkQueue instance_ = nullptr;
  uint32_t index_ = 0;
};

[[nodiscard]] constexpr VkQueue
queue::get() const noexcept
{
  return instance_;
}

[[nodiscard]] constexpr uint32_t
queue::index() const noexcept
{
  return index_;
}

template <typename Checker>
requires detail::result_checker<Checker> queue &
queue::present(VkPresentInfoKHR const & info, Checker && check)
{
  std::forward<Checker>(check)(vkQueuePresentKHR(get(), &info));
  return *this;
}

} // namespace mvk::types

#endif // MVK_TYPES_QUEUE_HPP_INCLUDED
