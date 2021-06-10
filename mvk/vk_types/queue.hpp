#ifndef MVK_VK_TYPES_QUEUE_HPP_INCLUDED
#define MVK_VK_TYPES_QUEUE_HPP_INCLUDED

#include "vk_types/common.hpp"
#include "vk_types/detail/checkers.hpp"

namespace mvk::vk_types
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
  wait_idle() noexcept;

  queue &
  submit(VkSubmitInfo const & submit_info);
  queue &
  submit(VkSubmitInfo const & submit_info, fence const & fence);

  template <typename Checker = decltype(detail::default_result_checker)>
  requires detail::result_checker<Checker> 
	queue &
	present(
		VkPresentInfoKHR const & present_info,
		Checker &&               check = detail::default_result_checker);

private:
  VkQueue  instance_ = nullptr;
  uint32_t index_    = 0;
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
requires detail::result_checker<Checker> 
queue &
queue::present(VkPresentInfoKHR const & present_info, Checker && check)
{
  std::forward<Checker>(check)(vkQueuePresentKHR(get(), &present_info));
  return *this;
}

} // namespace mvk::vk_types

#endif // MVK_VK_TYPES_QUEUE_HPP_INCLUDED
