#include "engine/draw.hpp"

#include "detail/misc.hpp"
#include "engine/allocate.hpp"
#include "engine/init.hpp"

namespace mvk::engine
{
  void recreate_after_swapchain_change( context & ctx ) noexcept
  {
    destroy_swapchain( ctx );
    ctx.swapchain_image_views.clear();
    init_swapchain( ctx );

    destroy_depth_image( ctx );
    init_depth_image( ctx );

    destroy_render_pass( ctx );
    init_render_pass( ctx );

    destroy_framebuffers( ctx );
    ctx.framebuffers.clear();
    init_framebuffers( ctx );

    destroy_pipelines( ctx );
    init_pipeline( ctx );

    destroy_sync( ctx );
    ctx.image_in_flight_fences.clear();
    init_sync( ctx );
  }

  void begin_draw( context & ctx ) noexcept
  {
    auto const image_available_semaphore = ctx.image_available_semaphores[ ctx.current_frame_index ];

    auto const current_image_index =
      detail::next_swapchain_image( ctx.device, ctx.swapchain, image_available_semaphore, nullptr );

    if ( !current_image_index.has_value() )
    {
      recreate_after_swapchain_change( ctx );
      begin_draw( ctx );
      return;
    }

    ctx.current_image_index = current_image_index.value();

    auto clear_color_value  = VkClearValue();
    clear_color_value.color = { { 0.0F, 0.0F, 0.0F, 1.0F } };

    auto clear_depth_value         = VkClearValue();
    clear_depth_value.depthStencil = { 1.0F, 0 };

    auto const clear_values = std::array{ clear_color_value, clear_depth_value };

    auto const render_pass_begin_info = [ &ctx, &clear_values ]
    {
      auto info                = VkRenderPassBeginInfo();
      info.sType               = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      info.renderPass          = ctx.render_pass;
      info.framebuffer         = ctx.framebuffers[ ctx.current_image_index ];
      info.renderArea.offset.x = 0;
      info.renderArea.offset.y = 0;
      info.renderArea.extent   = ctx.swapchain_extent;
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

    ctx.current_command_buffer = ctx.command_buffers[ ctx.current_buffer_index ];

    vkBeginCommandBuffer( ctx.current_command_buffer, &command_buffer_begin_info );
    vkCmdBeginRenderPass( ctx.current_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE );
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

    auto descriptor_sets = std::array{ uniform_set, ctx.image_descriptor_set_ };

    vkCmdBindPipeline( ctx.current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.pipeline );
    vkCmdBindVertexBuffers( ctx.current_command_buffer, 0, 1, &vertex_buffer, &vertex_offset );
    vkCmdBindIndexBuffer( ctx.current_command_buffer, index_buffer, index_offset, VK_INDEX_TYPE_UINT32 );
    vkCmdBindDescriptorSets( ctx.current_command_buffer,
                             VK_PIPELINE_BIND_POINT_GRAPHICS,
                             ctx.pipeline_layout,
                             0,
                             static_cast< uint32_t >( std::size( descriptor_sets ) ),
                             std::data( descriptor_sets ),
                             1,
                             &uniform_offset );
    vkCmdDrawIndexed( ctx.current_command_buffer, static_cast< uint32_t >( std::size( ctx.indices_ ) ), 1, 0, 0, 0 );
  }

  void end_draw( context & ctx ) noexcept
  {
    vkCmdEndRenderPass( ctx.current_command_buffer );
    vkEndCommandBuffer( ctx.current_command_buffer );
    // Wait for the image in flight to end if it is
    auto const * image_in_flight_fence = ctx.image_in_flight_fences[ ctx.current_image_index ];

    if ( image_in_flight_fence != nullptr )
    {
      vkWaitForFences( ctx.device, 1, image_in_flight_fence, VK_TRUE, std::numeric_limits< int64_t >::max() );
    }

    ctx.image_in_flight_fences[ ctx.current_image_index ] = &ctx.frame_in_flight_fences[ ctx.current_frame_index ];

    // get current semaphores
    auto const image_available_semaphore = ctx.image_available_semaphores[ ctx.current_frame_index ];
    auto const render_finished_semaphore = ctx.render_finished_semaphores[ ctx.current_frame_index ];

    auto const wait_semaphores   = std::array{ image_available_semaphore };
    auto const signal_semaphores = std::array{ render_finished_semaphore };
    auto const wait_stages     = std::array< VkPipelineStageFlags, 1 >{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    auto const command_buffers = std::array{ ctx.current_command_buffer };

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

    vkResetFences( ctx.device, 1, &ctx.frame_in_flight_fences[ ctx.current_frame_index ] );
    vkQueueSubmit( ctx.graphics_queue, 1, &submit_info, ctx.frame_in_flight_fences[ ctx.current_frame_index ] );

    auto const present_signal_semaphores = std::array{ render_finished_semaphore };
    auto const swapchains                = std::array{ ctx.swapchain };
    auto const image_indices             = std::array{ ctx.current_image_index };

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

    auto const present_result = vkQueuePresentKHR( ctx.present_queue, &present_info );
    vkQueueWaitIdle( ctx.present_queue );

    auto const change_swapchain =
      ( present_result == VK_ERROR_OUT_OF_DATE_KHR ) || ( present_result == VK_SUBOPTIMAL_KHR );

    if ( change_swapchain || ctx.framebuffer_resized )
    {
      ctx.framebuffer_resized = false;
      recreate_after_swapchain_change( ctx );
      return;
    }

    MVK_VERIFY( VK_SUCCESS == present_result );

    ctx.current_frame_index = ( ctx.current_frame_index + 1 ) % context::max_frames_in_flight;
    next_buffer( ctx );
  }

}  // namespace mvk::engine
