#include "Engine/DrawCallGenerator.hpp"

namespace Mvk::Engine {

[[nodiscard]] DrawCmd
DrawCallGenerator::create(Utility::Slice<vertex const> Vertices,
                          Utility::Slice<uint32_t const> Indices,
                          PVM const &Pvm, TexData TexInfo) noexcept {
  auto Call = DrawCmd();
  Call.Vtx = VBO.stage(Staging.map(Utility::as_bytes(Vertices)));
  Call.Idx = IBO.stage(Staging.map(Utility::as_bytes(Indices)));
  Call.Ubo = UBO.map(Utility::as_bytes(Pvm));
  Call.IdxCount = std::size(Indices);

  auto TexStage = Staging.map(Utility::as_bytes(TexInfo.Data));
  auto &Ctx = Staging.getContext();
  Texs.push_back(
      std::make_unique<Tex>(Ctx, TexStage, TexInfo.Width, TexInfo.Height));
  Call.Tex = Texs.back()->stage();
  return Call;
}

void DrawCallGenerator::updatePvm(PVM const &Pvm, DrawCmd &Cmd) noexcept {
  Cmd.Ubo = UBO.map(Utility::as_bytes(Pvm));
}

void DrawCallGenerator::nextBuffers() noexcept {
  Staging.nextBuffer();
  VBO.nextBuffer();
  IBO.nextBuffer();
  UBO.nextBuffer();
}

} // namespace Mvk::Engine
