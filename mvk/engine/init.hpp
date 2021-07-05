#ifndef MVK_ENGINE_initHPP_INCLUDED
#define MVK_ENGINE_initHPP_INCLUDED

#include "engine/context.hpp"

namespace mvk::engine
{
  void createContext(char const * Name, VkExtent2D Extent, InOut<Context> out) noexcept;

  void dtyContext(InOut<Context> Ctx) noexcept;

  void initWindow(InOut<Context> Ctx, VkExtent2D Extent) noexcept;
  void dtyWindow(InOut<Context> Ctx) noexcept;

  void initInst(InOut<Context> Ctx, char const * Name) noexcept;
  void dtyInst(InOut<Context> Ctx) noexcept;

  void initDbgMsngr(InOut<Context> Ctx) noexcept;
  void dtyDbgMsngr(InOut<Context> Ctx) noexcept;

  void initSurf(InOut<Context> Ctx) noexcept;
  void dtySurf(InOut<Context> Ctx) noexcept;

  void selectPhysicalDev(InOut<Context> Ctx) noexcept;
  void selectSurfFmt(InOut<Context> Ctx) noexcept;

  void initDev(InOut<Context> Ctx) noexcept;
  void dtyDev(InOut<Context> Ctx) noexcept;

  void initLays(InOut<Context> Ctx) noexcept;
  void dtyLays(InOut<Context> Ctx) noexcept;

  void initPools(InOut<Context> Ctx) noexcept;
  void dtyPools(InOut<Context> Ctx) noexcept;

  void initSwapchain(InOut<Context> Ctx) noexcept;
  void dtySwapchain(InOut<Context> Ctx) noexcept;

  void initDepthImg(InOut<Context> Ctx) noexcept;
  void dtyDepthImg(InOut<Context> Ctx) noexcept;

  void initFramebuffers(InOut<Context> Ctx) noexcept;
  void dtyFramebuffers(InOut<Context> Ctx) noexcept;

  void initRdrPass(InOut<Context> Ctx) noexcept;
  void dtyRdrPass(InOut<Context> Ctx) noexcept;

  void initDoesntBelongHere(InOut<Context> Ctx) noexcept;
  void dtyDoesntBelongHere(InOut<Context> Ctx) noexcept;

  void initCmdBuffs(InOut<Context> Ctx) noexcept;
  void dtyCmdBuffs(InOut<Context> Ctx) noexcept;

  void initShaders(InOut<Context> Ctx) noexcept;
  void dtyShaders(InOut<Context> Ctx) noexcept;

  void initSamplers(InOut<Context> Ctx) noexcept;
  void dtySamplers(InOut<Context> Ctx) noexcept;

  void initPipeline(InOut<Context> Ctx) noexcept;
  void dtyPipelines(InOut<Context> Ctx) noexcept;

  void initSync(InOut<Context> Ctx) noexcept;
  void dtySync(InOut<Context> Ctx) noexcept;
}  // namespace mvk::engine

#endif
