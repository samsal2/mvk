#ifndef MVK_ENGINE_initHPP_INCLUDED
#define MVK_ENGINE_initHPP_INCLUDED

#include "engine/Context.hpp"

namespace mvk::engine
{
  [[nodiscard]] Context createContext( char const * Name, VkExtent2D Extent ) noexcept;

  void destroyContext( Context & Ctx ) noexcept;

  void initWindow( Context & Ctx, VkExtent2D Extent ) noexcept;
  void destroyWindow( Context & Ctx ) noexcept;

  void initInst( Context & Ctx, char const * Name ) noexcept;
  void destroyInst( Context & Ctx ) noexcept;

  void initDbgMsngr( Context & Ctx ) noexcept;
  void destroyDbgMsngr( Context & Ctx ) noexcept;

  void initSurf( Context & Ctx ) noexcept;
  void destroySurf( Context & Ctx ) noexcept;

  void selectPhysicalDev( Context & Ctx ) noexcept;
  void selectSurfFmt( Context & Ctx ) noexcept;

  void initDev( Context & Ctx ) noexcept;
  void destroyDev( Context & Ctx ) noexcept;

  void initLays( Context & Ctx ) noexcept;
  void destroyLays( Context & Ctx ) noexcept;

  void initPools( Context & Ctx ) noexcept;
  void destroyPools( Context & Ctx ) noexcept;

  void initSwapchain( Context & Ctx ) noexcept;
  void destroySwapchain( Context & Ctx ) noexcept;

  void initDepthImg( Context & Ctx ) noexcept;
  void destroyDepthImg( Context & Ctx ) noexcept;

  void initFramebuffers( Context & Ctx ) noexcept;
  void destroyFramebuffers( Context & Ctx ) noexcept;

  void initRdrPass( Context & Ctx ) noexcept;
  void destroyRdrPass( Context & Ctx ) noexcept;

  void initDoesntBelongHere( Context & Ctx ) noexcept;
  void destroyDoesntBelongHere( Context & Ctx ) noexcept;

  void initCmdBuffs( Context & Ctx ) noexcept;
  void destroyCmdBuffs( Context & Ctx ) noexcept;

  void initShaders( Context & Ctx ) noexcept;
  void destroyShaders( Context & Ctx ) noexcept;

  void initSamplers( Context & Ctx ) noexcept;
  void destroySamplers( Context & Ctx ) noexcept;

  void initPipeline( Context & Ctx ) noexcept;
  void destroyPipelines( Context & Ctx ) noexcept;

  void initSync( Context & Ctx ) noexcept;
  void destroySync( Context & Ctx ) noexcept;
}  // namespace mvk::engine

#endif
