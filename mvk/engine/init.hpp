#ifndef MVK_ENGINE_INIT_HPP_INCLUDED
#define MVK_ENGINE_INIT_HPP_INCLUDED

#include "engine/context.hpp"

namespace mvk::engine
{
  [[nodiscard]] context create_context( std::string const & name, types::window::extent extent ) noexcept;

  void init_window( context & ctx, types::window::extent extent ) noexcept;
  void init_instance( context & ctx, std::string const & name ) noexcept;
  void init_debug_messenger( context & ctx ) noexcept;
  void init_surface( context & ctx ) noexcept;
  void select_physical_device( context & ctx ) noexcept;
  void select_surface_format( context & ctx ) noexcept;
  void init_device( context & ctx ) noexcept;
  void init_layouts( context & ctx ) noexcept;
  void init_pools( context & ctx ) noexcept;
  void init_swapchain( context & ctx ) noexcept;
  void init_depth_image( context & ctx ) noexcept;
  void init_framebuffers( context & ctx ) noexcept;
  void init_main_render_pass( context & ctx ) noexcept;
  void init_doesnt_belong_here( context & ctx ) noexcept;
  void init_command_buffers( context & ctx ) noexcept;
  void init_shaders( context & ctx ) noexcept;
  void init_samplers( context & ctx ) noexcept;
  void init_pipeline( context & ctx ) noexcept;
  void init_sync( context & ctx ) noexcept;
}  // namespace mvk::engine

#endif
