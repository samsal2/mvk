#ifndef MVK_DETAIL_READERS_HPP_INCLUDED
#define MVK_DETAIL_READERS_HPP_INCLUDED

#include "vk_types/vk_types.hpp"

#include <filesystem>
#include <utility>
#include <vector>

namespace mvk::detail
{

[[nodiscard]] std::pair<std::vector<vertex>, std::vector<uint32_t>>
read_object(std::filesystem::path const & path);

[[nodiscard]] std::vector<char>
read_file(std::filesystem::path const & path);

} // namespace mvk::detail

#endif
