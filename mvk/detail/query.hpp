#ifndef MVK_DETAIL_QUERY_HPP_INCLUDED
#define MVK_DETAIL_QUERY_HPP_INCLUDED

#include <cstddef>

namespace mvk::detail
{

template <auto Call, typename = decltype(Call)>
struct query;
template <auto Call, typename Requirement1, typename Requirement2,
          typename Desired>
struct query<Call, void (*)(Requirement1, Requirement2, Desired *)>
{
  [[nodiscard]] static constexpr auto
  with(Requirement1 requirement1, Requirement2 requirement2) noexcept
  {
    auto desired = Desired();
    Call(requirement1, requirement2, &desired);
    return desired;
  }
};

template <auto Call, typename Requirement1, typename Requirement2,
          typename Desired>
struct query<Call, VkResult (*)(Requirement1, Requirement2, Desired *)>
{
  [[nodiscard]] static constexpr auto
  with(Requirement1 requirement1, Requirement2 requirement2) noexcept
  {
    auto desired = Desired();
    Call(requirement1, requirement2, &desired);
    return desired;
  }
};
template <auto Call, typename Requirement1, typename Requirement2,
          typename Desired>
struct query<Call,
             VkResult (*)(Requirement1, Requirement2, uint32_t *, Desired *)>
{
  [[nodiscard]] static constexpr auto
  with(Requirement1 requirement1, Requirement2 requirement2) noexcept
  {
    auto count = uint32_t(0);
    Call(requirement1, requirement2, &count, nullptr);

    auto buffer = std::vector<Desired>(count);
    Call(requirement1, requirement2, &count, std::data(buffer));
    return buffer;
  }
};

template <auto Call, typename Requirement, typename Desired>
struct query<Call, VkResult (*)(Requirement, uint32_t *, Desired *)>
{
  [[nodiscard]] static constexpr auto
  with(Requirement requirement) noexcept
  {
    auto count = uint32_t(0);
    Call(requirement, &count, nullptr);

    auto buffer = std::vector<Desired>(count);
    Call(requirement, &count, std::data(buffer));
    return buffer;
  }
};

template <auto Call, typename Requirement, typename Desired>
struct query<Call, void (*)(Requirement, uint32_t *, Desired *)>
{
  [[nodiscard]] static constexpr auto
  with(Requirement requirement) noexcept
  {
    auto count = uint32_t(0);
    Call(requirement, &count, nullptr);

    auto buffer = std::vector<Desired>(count);
    Call(requirement, &count, std::data(buffer));
    return buffer;
  }
};

template <auto Call, typename Requirement, typename Desired>
struct query<Call, void (*)(Requirement, Desired *)>
{
  [[nodiscard]] static constexpr auto
  with(Requirement requirement) noexcept
  {
    auto desired = Desired();
    Call(requirement, &desired);
    return desired;
  }
};

template <auto Call, typename Desired>
struct query<Call, VkResult (*)(uint32_t *, Desired *)>
{
  [[nodiscard]] static constexpr auto
  with() noexcept
  {
    auto count = uint32_t(0);
    Call(&count, nullptr);
    auto desired = std::vector<Desired>(count);
    Call(&count, std::data(desired));
    return desired;
  }
};

} // namespace mvk::detail

#endif
