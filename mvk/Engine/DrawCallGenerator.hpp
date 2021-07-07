#ifndef MVK_ENGINE_DRAWCALLGENERATOR_HPP_INCLUDED
#define MVK_ENGINE_DRAWCALLGENERATOR_HPP_INCLUDED

#include "Engine/IdxBuff.hpp"
#include "Engine/StagingBuff.hpp"
#include "Engine/Tex.hpp"
#include "Engine/UboBuff.hpp"
#include "Engine/VtxBuff.hpp"
#include "glm/gtc/constants.hpp"

namespace Mvk::Engine {

struct DrawCmd {
  VtxBuff::StageResult Vtx;
  IdxBuff::StageResult Idx;
  UboBuff::MapResult Ubo;
  Tex::StageResult Tex;
  uint32_t IdxCount;
};

struct TexData {
  uint32_t Width;
  uint32_t Height;
  Utility::Slice<unsigned char const> Data;
};

class DrawCallGenerator {
public:
  static constexpr auto DefaultBuffSize = 1024;

  explicit DrawCallGenerator(Context &Ctx) noexcept
      : Staging(Ctx, DefaultBuffSize), VBO(Ctx, DefaultBuffSize),
        IBO(Ctx, DefaultBuffSize), UBO(Ctx, DefaultBuffSize) {}

  [[nodiscard]] DrawCmd create(Utility::Slice<vertex const> Vertices,
                               Utility::Slice<uint32_t const> Indices,
                               PVM const &Pvm, TexData TexInfo) noexcept;

  void updatePvm(PVM const &Pvm, DrawCmd &Cmd) noexcept;
  void nextBuffers() noexcept;

private:
  StagingBuff Staging;
  VtxBuff VBO;
  IdxBuff IBO;
  UboBuff UBO;

  // TODO(samuel): Temporary
  std::vector<std::unique_ptr<Tex>> Texs;
};

} // namespace Mvk::Engine

#endif
