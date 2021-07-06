#include "Detail/Readers.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#pragma clang diagnostic pop

#include "Utility/Verify.hpp"

namespace Mvk::Detail {

[[nodiscard]] std::pair<std::vector<vertex>, std::vector<uint32_t>>
readObj(std::filesystem::path const &Path) noexcept {
  auto Attr = tinyobj::attrib_t();
  auto Shapes = std::vector<tinyobj::shape_t>();
  auto Mat = std::vector<tinyobj::material_t>();
  auto Warn = std::string();
  auto Err = std::string();

  [[maybe_unused]] auto const Result =
      tinyobj::LoadObj(&Attr, &Shapes, &Mat, &Warn, &Err, Path.c_str());

  MVK_VERIFY(Result);

  auto Vtxs = std::vector<vertex>();
  auto Idxs = std::vector<uint32_t>();

  for (auto const &Shape : Shapes) {
    for (auto const &Idx : Shape.mesh.indices) {
      Idxs.push_back(static_cast<uint32_t>(std::size(Idxs)));

      auto Vtx = vertex();
      auto const VtxIdx = static_cast<size_t>(Idx.vertex_index);

      Vtx.pos = [&Attr, VtxIdx] {
        auto const X = Attr.vertices[3 * VtxIdx + 0];
        auto const Y = Attr.vertices[3 * VtxIdx + 1];
        auto const Z = Attr.vertices[3 * VtxIdx + 2];
        return glm::vec3(X, Y, Z);
      }();

      Vtx.color = glm::vec3(1.0F, 1.0F, 1.0F);

      auto const TexCoordIdx = static_cast<size_t>(Idx.texcoord_index);

      Vtx.texture_coord = [&Attr, &TexCoordIdx] {
        auto const X = Attr.texcoords[2 * TexCoordIdx + 0];
        auto const Y = Attr.texcoords[2 * TexCoordIdx + 1];
        return glm::vec2(X, 1 - Y);
      }();

      Vtxs.push_back(Vtx);
    }
  }

  return std::make_pair(Vtxs, Idxs);
}

[[nodiscard]] std::vector<char>
readFile(std::filesystem::path const &Path) noexcept {
  MVK_VERIFY(std::filesystem::exists(Path));
  auto File = std::ifstream(Path, std::ios::ate | std::ios::binary);
  auto Buff = std::vector<char>(static_cast<size_t>(File.tellg()));

  File.seekg(0);
  File.read(std::data(Buff), static_cast<int64_t>(std::size(Buff)));
  File.close();

  return Buff;
}

} // namespace Mvk::Detail
