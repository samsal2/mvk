#ifndef MVK_DETAIL_READERS_HPP_INCLUDED
#define MVK_DETAIL_READERS_HPP_INCLUDED

#include "shader_types.hpp"
#include "types/types.hpp"

#include <filesystem>
#include <utility>
#include <vector>

namespace mvk::detail
{
[[nodiscard]] std::pair<std::vector<vertex>, std::vector<u32>>
read_object(std::filesystem::path const & path) noexcept;

[[nodiscard]] std::vector<char>
read_file(std::filesystem::path const & path) noexcept;

} // namespace mvk::detail

#endif
