#include "detail/readers.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#pragma clang diagnostic pop

namespace mvk::detail
{
[[nodiscard]] std::pair<std::vector<vertex>, std::vector<uint32_t>>
read_object(std::filesystem::path const & path) noexcept
{
  auto attrib = tinyobj::attrib_t();
  auto shapes = std::vector<tinyobj::shape_t>();
  auto materials = std::vector<tinyobj::material_t>();
  auto warn = std::string();
  auto error = std::string();

  [[maybe_unused]] auto const success = tinyobj::LoadObj(
      &attrib, &shapes, &materials, &warn, &error, path.c_str());

  MVK_VERIFY(success);

  auto vertices = std::vector<vertex>();
  auto indices = std::vector<uint32_t>();

  for (auto const & shape : shapes)
  {
    for (auto const & index : shape.mesh.indices)
    {
      indices.push_back(static_cast<uint32_t>(std::size(indices)));

      auto vtx = vertex();
      auto const vertex_index = static_cast<size_t>(index.vertex_index);

      vtx.pos = [&attrib, vertex_index]
      {
        auto const x = attrib.vertices[3 * vertex_index + 0];
        auto const y = attrib.vertices[3 * vertex_index + 1];
        auto const z = attrib.vertices[3 * vertex_index + 2];
        return glm::vec3(x, y, z);
      }();

      vtx.color = glm::vec3(1.0F, 1.0F, 1.0F);

      auto const texture_coordinates_index =
          static_cast<size_t>(index.texcoord_index);

      vtx.texture_coord = [&attrib, &texture_coordinates_index]
      {
        auto const x = attrib.texcoords[2 * texture_coordinates_index + 0];
        auto const y = attrib.texcoords[2 * texture_coordinates_index + 1];
        return glm::vec2(x, 1 - y);
      }();

      vertices.push_back(vtx);
    }
  }

  return std::make_pair(vertices, indices);
}

[[nodiscard]] std::vector<char>
read_file(std::filesystem::path const & path) noexcept
{
  MVK_VERIFY(std::filesystem::exists(path));
  auto file = std::ifstream(path, std::ios::ate | std::ios::binary);
  auto buffer = std::vector<char>(static_cast<size_t>(file.tellg()));

  file.seekg(0);
  file.read(std::data(buffer), static_cast<int64_t>(std::size(buffer)));
  file.close();

  return buffer;
}

} // namespace mvk::detail
