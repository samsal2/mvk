#ifndef MVK_VK_TYPES_DETAIL_GET_CONTAINER_HPP_INCLUDED
#define MVK_VK_TYPES_DETAIL_GET_CONTAINER_HPP_INCLUDED

#include <vector>

namespace mvk::vk_types::detail
{

template <typename T, bool IsAllocation>
struct get_container
{
        using type = std::vector<T>;
};

template <typename T>
struct get_container<T, false>
{
        using type = T;
};

template <typename T, bool IsAllocation>
using get_container_t = typename get_container<T, IsAllocation>::type;

} // namespace mvk::vk_types::detail

#endif
