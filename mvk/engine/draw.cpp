#include "engine/draw.hpp"

#include "detail/misc.hpp"
#include "engine/allocate.hpp"
#include "engine/init.hpp"

namespace mvk::engine
{
  void recreate_after_swapchain_change( context & ctx ) noexcept
  {
    ctx.swapchain_image_views_.clear();

    init_swapchain( ctx );
    init_depth_image( ctx );
    init_main_render_pass( ctx );

    ctx.framebuffers_.clear();
    init_framebuffers( ctx );

    init_pipeline( ctx );
    ctx.image_in_flight_fences_.clear();
    init_sync( ctx );
  }

  void begin_draw( context & ctx ) noexcept
  {
    auto const & image_available_semaphore = ctx.image_available_semaphores_[ ctx.current_frame_index_ ];

    auto const current_image_index = detail::next_swapchain_image(
      types::get( ctx.device_ ), types::get( ctx.swapchain_ ), types::get( image_available_semaphore ), nullptr );

    if ( !current_image_index.has_value() )
    {
      recreate_after_swapchain_change( ctx );
      begin_draw( ctx );
      return;
    }

    ctx.current_image_index_ = current_image_index.value();

    auto clear_color_value  = VkClearValue();
    clear_color_value.color = { { 0.0F, 0.0F, 0.0F, 1.0F } };

    auto clear_depth_value         = VkClearValue();
    clear_depth_value.depthStencil = { 1.0F, 0 };

    auto const clear_values = std::array{ clear_color_value, clear_depth_value };

    auto const render_pass_begin_info = [ &ctx, &clear_values ]
    {
      auto info                = VkRenderPassBeginInfo();
      info.sType               = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      info.renderPass          = types::get( ctx.render_pass_ );
      info.framebuffer         = types::get( ctx.framebuffers_[ ctx.current_image_index_ ] );
      info.renderArea.offset.x = 0;
      info.renderArea.offset.y = 0;
      info.renderArea.extent   = ctx.swapchain_extent_;
      info.clearValueCount     = static_cast< uint32_t >( std::size( clear_values ) );
      info.pClearValues        = std::data( clear_values );
      return info;
    }();

    auto const command_buffer_begin_info = []
    {
      auto info             = VkCommandBufferBeginInfo();
      info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      info.flags            = 0;
      info.pInheritanceInfo = nullptr;
      return info;
    }();

    ctx.current_command_buffer_ = types::decay( ctx.command_buffers_[ ctx.current_buffer_index_ ] );

    vkBeginCommandBuffer( types::get( ctx.current_command_buffer_ ), &command_buffer_begin_info );
    vkCmdBeginRenderPass(
      types::get( ctx.current_command_buffer_ ), &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE );
  }

  void basic_draw( context &                         ctx,
                   utility::slice< std::byte const > vertices,
                   utility::slice< std::byte const > indices,
                   utility::slice< std::byte const > pvm ) noexcept
  {
    auto vertex_stage                     = staging_allocate( ctx, vertices );
    auto [ vertex_buffer, vertex_offset ] = vertex_allocate( ctx, vertex_stage );

    auto index_stage                    = staging_allocate( ctx, indices );
    auto [ index_buffer, index_offset ] = index_allocate( ctx, index_stage );

    auto [ uniform_set, uniform_offset ] = uniform_allocate( ctx, utility::as_bytes( pvm ) );

    auto descriptor_sets = std::array{ types::get( uniform_set ), types::get( ctx.image_descriptor_set_ ) };

    vkCmdBindPipeline(
      types::get( ctx.current_command_buffer_ ), VK_PIPELINE_BIND_POINT_GRAPHICS, types::get( ctx.pipeline_ ) );
    vkCmdBindVertexBuffers(
      types::get( ctx.current_command_buffer_ ), 0, 1, &types::get( vertex_buffer ), &vertex_offset );
    vkCmdBindIndexBuffer(
      types::get( ctx.current_command_buffer_ ), types::get( index_buffer ), index_offset, VK_INDEX_TYPE_UINT32 );
    vkCmdBindDescriptorSets( types::get( ctx.current_command_buffer_ ),
                             VK_PIPELINE_BIND_POINT_GRAPHICS,
                             types::get( ctx.pipeline_layout_ ),
                             0,
                             static_cast< uint32_t >( std::size( descriptor_sets ) ),
                             std::data( descriptor_sets ),
                             1,
                             &uniform_offset );
    vkCmdDrawIndexed(
      types::get( ctx.current_command_buffer_ ), static_cast< uint32_t >( std::size( ctx.indices_ ) ), 1, 0, 0, 0 );
  }

  void end_draw( context & ctx ) noexcept
  {
    vkCmdEndRenderPass( types::get( ctx.current_command_buffer_ ) );
    vkEndCommandBuffer( types::get( ctx.current_command_buffer_ ) );
    // Wait for the image in flight to end if it is
    auto const * image_in_flight_fence = ctx.image_in_flight_fences_[ ctx.current_image_index_ ];

    if ( image_in_flight_fence != nullptr )
    {
      vkWaitForFences( types::get( ctx.device_ ),
                       1,
                       &types::get( *image_in_flight_fence ),
                       VK_TRUE,
                       std::numeric_limits< int64_t >::max() );
    }

    ctx.image_in_flight_fences_[ ctx.current_image_index_ ] = &ctx.frame_in_flight_fences_[ ctx.current_frame_index_ ];

    // get current semaphores
    auto const image_available_semaphore = types::decay( ctx.image_available_semaphores_[ ctx.current_frame_index_ ] );
    auto const render_finished_semaphore = types::decay( ctx.render_finished_semaphores_[ ctx.current_frame_index_ ] );

    auto const wait_semaphores   = std::array{ types::get( image_available_semaphore ) };
    auto const signal_semaphores = std::array{ types::get( render_finished_semaphore ) };
    auto const wait_stages     = std::array< VkPipelineStageFlags, 1 >{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    auto const command_buffers = std::array{ types::get( ctx.current_command_buffer_ ) };

    auto const submit_info = [ &wait_semaphores, &signal_semaphores, &wait_stages, &command_buffers ]
    {
      auto info                 = VkSubmitInfo();
      info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      info.waitSemaphoreCount   = static_cast< uint32_t >( std::size( wait_semaphores ) );
      info.pWaitSemaphores      = std::data( wait_semaphores );
      info.pWaitDstStageMask    = std::data( wait_stages );
      info.commandBufferCount   = static_cast< uint32_t >( std::size( command_buffers ) );
      info.pCommandBuffers      = std::data( command_buffers );
      info.signalSemaphoreCount = static_cast< uint32_t >( std::size( signal_semaphores ) );
      info.pSignalSemaphores    = std::data( signal_semaphores );
      return info;
    }();

    vkResetFences(
      types::get( ctx.device_ ), 1, &types::get( ctx.frame_in_flight_fences_[ ctx.current_frame_index_ ] ) );
    vkQueueSubmit( types::get( ctx.graphics_queue_ ),
                   1,
                   &submit_info,
                   types::get( ctx.frame_in_flight_fences_[ ctx.current_frame_index_ ] ) );

    auto const present_signal_semaphores = std::array{ types::get( render_finished_semaphore ) };
    auto const swapchains                = std::array{ types::get( ctx.swapchain_ ) };
    auto const image_indices             = std::array{ ctx.current_image_index_ };

    auto const present_info = [ &present_signal_semaphores, &swapchains, &image_indices ]
    {
      auto info               = VkPresentInfoKHR();
      info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
      info.waitSemaphoreCount = static_cast< uint32_t >( std::size( present_signal_semaphores ) );
      info.pWaitSemaphores    = std::data( present_signal_semaphores );
      info.swapchainCount     = static_cast< uint32_t >( std::size( swapchains ) );
      info.pSwapchains        = std::data( swapchains );
      info.pImageIndices      = std::data( image_indices );
      info.pResults           = nullptr;
      return info;
    }();

    auto const present_result = vkQueuePresentKHR( types::get( ctx.present_queue_ ), &present_info );
    vkQueueWaitIdle( types::get( ctx.present_queue_ ) );

    auto const resized = ctx.window_.framebuffer_resized();
    auto const change_swapchain =
      ( present_result == VK_ERROR_OUT_OF_DATE_KHR ) || ( present_result == VK_SUBOPTIMAL_KHR );

    if ( change_swapchain || resized )
    {
      ctx.window_.set_framebuffer_resized( false );
      recreate_after_swapchain_change( ctx );
      return;
    }

    MVK_VERIFY( VK_SUCCESS == present_result );

    ctx.current_frame_index_ = ( ctx.current_frame_index_ + 1 ) % context::max_frames_in_flight;
    next_buffer( ctx );
  }

}  // namespace mvk::engine
