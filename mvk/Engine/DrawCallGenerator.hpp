#pragma once

#include "Engine/IboMgr.hpp"
#include "Engine/StagingMgr.hpp"
#include "Engine/Tex.hpp"
#include "Engine/UboMgr.hpp"
#include "Engine/VboMgr.hpp"
#include "glm/gtc/constants.hpp"

namespace Mvk::Engine {

struct DrawCmd {
  VboMgr::StageResult Vtx;
  IboMgr::StageResult Idx;
  UboMgr::MapResult Ubo;
  Tex::StageResult Tex;
  uint32_t IdxCount;
};

struct TexData {
  uint32_t Width;
  uint32_t Height;
  std::span<unsigned char const> Data;
};

class DrawCallGenerator {
public:
  static constexpr auto DefaultBuffSize = 1024;

  explicit DrawCallGenerator(Context &Ctx) noexcept
      : Staging(Ctx, DefaultBuffSize), VBO(Ctx, DefaultBuffSize),
        IBO(Ctx, DefaultBuffSize), UBO(Ctx, DefaultBuffSize) {}

  [[nodiscard]] DrawCmd create(std::span<vertex const> Vertices,
                               std::span<uint32_t const> Indices,
                               PVM const &Pvm, TexData TexInfo) noexcept;

  void updatePvm(PVM const &Pvm, DrawCmd &Cmd) noexcept;
  void nextBuffers() noexcept;

private:
  StagingMgr Staging;
  VboMgr VBO;
  IboMgr IBO;
  UboMgr UBO;

  // TODO(samuel): Temporary
  std::vector<std::unique_ptr<Tex>> Texs;
};

} // namespace Mvk::Engine
