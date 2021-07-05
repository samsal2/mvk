#ifndef MVK_DETAIL_READERS_HPP_INCLUDED
#define MVK_DETAIL_READERS_HPP_INCLUDED

#include "shader_types.hpp"

#include <filesystem>
#include <utility>
#include <vector>

namespace mvk::detail
{
  [[nodiscard]] std::pair<std::vector<vertex>, std::vector<uint32_t>>
    readObj(std::filesystem::path const & Path) noexcept;

  [[nodiscard]] std::vector<char> readFile(std::filesystem::path const & Path) noexcept;

}  // namespace mvk::detail

#endif
