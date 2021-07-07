#include "Engine/DrawCallGenerator.hpp"

namespace Mvk::Engine {

[[nodiscard]] DrawCmd
DrawCallGenerator::create(std::span<vertex const> Vertices,
                          std::span<uint32_t const> Indices, PVM const &Pvm,
                          TexData TexInfo) noexcept {
  auto Call = DrawCmd();
  Call.Vtx = VBO.stage(Staging.map(std::as_bytes(Vertices)));
  Call.Idx = IBO.stage(Staging.map(std::as_bytes(Indices)));
  Call.Ubo = UBO.map({reinterpret_cast<std::byte const *>(&Pvm), sizeof(Pvm)});
  Call.IdxCount = std::size(Indices);

  auto TexStage = Staging.map(std::as_bytes(TexInfo.Data));
  auto &Ctx = Staging.getContext();
  Texs.push_back(
      std::make_unique<Tex>(Ctx, TexStage, TexInfo.Width, TexInfo.Height));
  Call.Tex = Texs.back()->stage();
  return Call;
}

void DrawCallGenerator::updatePvm(PVM const &Pvm, DrawCmd &Cmd) noexcept {
  Cmd.Ubo = UBO.map({reinterpret_cast<std::byte const *>(&Pvm), sizeof(Pvm)});
}

void DrawCallGenerator::nextBuffers() noexcept {
  Staging.nextBuffer();
  VBO.nextBuffer();
  IBO.nextBuffer();
  UBO.nextBuffer();
}

} // namespace Mvk::Engine
