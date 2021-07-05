#include "engine/draw.hpp"

#include "detail/misc.hpp"
#include "engine/allocate.hpp"
#include "engine/init.hpp"

namespace mvk::engine
{
  void recreateAfterSwapchainChange( Context & Ctx ) noexcept
  {
    destroySwapchain( Ctx );
    Ctx.SwapchainImgViews.clear();
    initSwapchain( Ctx );

    destroyDepthImg( Ctx );
    initDepthImg( Ctx );

    destroyRdrPass( Ctx );
    initRdrPass( Ctx );

    destroyFramebuffers( Ctx );
    Ctx.Framebuffers.clear();
    initFramebuffers( Ctx );

    destroyPipelines( Ctx );
    initPipeline( Ctx );

    destroySync( Ctx );
    Ctx.ImgInFlightFences.clear();
    initSync( Ctx );
  }

  void beginDraw( Context & Ctx ) noexcept
  {
    auto const ImgAvailableSemaphores = Ctx.ImgAvailableSemaphores[ Ctx.CurrentFrameIdx ];

    auto const CurrentImgIdx = detail::querySwapchainImg( Ctx.Dev, Ctx.Swapchain, ImgAvailableSemaphores, nullptr );

    if ( !CurrentImgIdx.has_value() )
    {
      recreateAfterSwapchainChange( Ctx );
      beginDraw( Ctx );
      return;
    }

    Ctx.CurrentImgIdx = CurrentImgIdx.value();

    auto ClrColorVal  = VkClearValue();
    ClrColorVal.color = { { 0.0F, 0.0F, 0.0F, 1.0F } };

    auto ClrDepthVal         = VkClearValue();
    ClrDepthVal.depthStencil = { 1.0F, 0 };

    auto const ClrVals = std::array{ ClrColorVal, ClrDepthVal };

    auto RdrPassBeginInfo                = VkRenderPassBeginInfo();
    RdrPassBeginInfo.sType               = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RdrPassBeginInfo.renderPass          = Ctx.RdrPass;
    RdrPassBeginInfo.framebuffer         = Ctx.Framebuffers[ Ctx.CurrentImgIdx ];
    RdrPassBeginInfo.renderArea.offset.x = 0;
    RdrPassBeginInfo.renderArea.offset.y = 0;
    RdrPassBeginInfo.renderArea.extent   = Ctx.SwapchainExtent;
    RdrPassBeginInfo.clearValueCount     = static_cast< uint32_t >( std::size( ClrVals ) );
    RdrPassBeginInfo.pClearValues        = std::data( ClrVals );

    auto CmdBuffBeginInfo             = VkCommandBufferBeginInfo();
    CmdBuffBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CmdBuffBeginInfo.flags            = 0;
    CmdBuffBeginInfo.pInheritanceInfo = nullptr;

    Ctx.CurrentCmdBuff = Ctx.CmdBuffs[ Ctx.CurrentBuffIdx ];

    vkBeginCommandBuffer( Ctx.CurrentCmdBuff, &CmdBuffBeginInfo );
    vkCmdBeginRenderPass( Ctx.CurrentCmdBuff, &RdrPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
  }

  void basicDraw( Context &                         Ctx,
                  utility::Slice< std::byte const > Vtxs,
                  utility::Slice< std::byte const > Idxs,
                  utility::Slice< std::byte const > pvm ) noexcept
  {
    auto VtxStage            = allocStaging( Ctx, Vtxs );
    auto [ VtxBuff, VtxOff ] = allocVtx( Ctx, VtxStage );

    auto IdxStage            = allocStaging( Ctx, Idxs );
    auto [ IdxBuff, IdxOff ] = allocIdx( Ctx, IdxStage );

    auto [ UboSet, UboOff ] = allocUbo( Ctx, utility::as_bytes( pvm ) );

    auto DescriptorSets = std::array{ UboSet, Ctx.image_descriptor_set_ };

    vkCmdBindPipeline( Ctx.CurrentCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, Ctx.Pipeline );
    vkCmdBindVertexBuffers( Ctx.CurrentCmdBuff, 0, 1, &VtxBuff, &VtxOff );
    vkCmdBindIndexBuffer( Ctx.CurrentCmdBuff, IdxBuff, 0, VK_INDEX_TYPE_UINT32 );
    vkCmdBindDescriptorSets( Ctx.CurrentCmdBuff,
                             VK_PIPELINE_BIND_POINT_GRAPHICS,
                             Ctx.PipelineLay,
                             0,
                             static_cast< uint32_t >( std::size( DescriptorSets ) ),
                             std::data( DescriptorSets ),
                             1,
                             &UboOff );
    vkCmdDrawIndexed( Ctx.CurrentCmdBuff, static_cast< uint32_t >( std::size( Ctx.indices_ ) ), 1, 0, 0, 0 );
  }

  void endDraw( Context & Ctx ) noexcept
  {
    vkCmdEndRenderPass( Ctx.CurrentCmdBuff );
    vkEndCommandBuffer( Ctx.CurrentCmdBuff );
    // Wait for the Img in flight to end if it is
    auto const * ImgInFlightFence = Ctx.ImgInFlightFences[ Ctx.CurrentImgIdx ];

    if ( ImgInFlightFence != nullptr )
    {
      vkWaitForFences( Ctx.Dev, 1, ImgInFlightFence, VK_TRUE, std::numeric_limits< int64_t >::max() );
    }

    Ctx.ImgInFlightFences[ Ctx.CurrentImgIdx ] = &Ctx.FrameInFlightFences[ Ctx.CurrentFrameIdx ];

    // get current semaphores
    auto const ImgAvailableSemaphores = Ctx.ImgAvailableSemaphores[ Ctx.CurrentFrameIdx ];
    auto const RdrFinishedSemaphore   = Ctx.RdrFinishedSemaphores[ Ctx.CurrentFrameIdx ];

    auto const WaitSemaphore = std::array{ ImgAvailableSemaphores };
    auto const SigSemaphore  = std::array{ RdrFinishedSemaphore };
    auto const WaitStages    = std::array< VkPipelineStageFlags, 1 >{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    auto const CmdBuffs      = std::array{ Ctx.CurrentCmdBuff };

    auto SubmitInfo                 = VkSubmitInfo();
    SubmitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.waitSemaphoreCount   = static_cast< uint32_t >( std::size( WaitSemaphore ) );
    SubmitInfo.pWaitSemaphores      = std::data( WaitSemaphore );
    SubmitInfo.pWaitDstStageMask    = std::data( WaitStages );
    SubmitInfo.commandBufferCount   = static_cast< uint32_t >( std::size( CmdBuffs ) );
    SubmitInfo.pCommandBuffers      = std::data( CmdBuffs );
    SubmitInfo.signalSemaphoreCount = static_cast< uint32_t >( std::size( SigSemaphore ) );
    SubmitInfo.pSignalSemaphores    = std::data( SigSemaphore );

    vkResetFences( Ctx.Dev, 1, &Ctx.FrameInFlightFences[ Ctx.CurrentFrameIdx ] );
    vkQueueSubmit( Ctx.GfxQueue, 1, &SubmitInfo, Ctx.FrameInFlightFences[ Ctx.CurrentFrameIdx ] );

    auto const present_signal_semaphores = std::array{ RdrFinishedSemaphore };
    auto const Swapchains                = std::array{ Ctx.Swapchain };
    auto const Img_Idxs                  = std::array{ Ctx.CurrentImgIdx };

    auto PresentInfo               = VkPresentInfoKHR();
    PresentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    PresentInfo.waitSemaphoreCount = static_cast< uint32_t >( std::size( present_signal_semaphores ) );
    PresentInfo.pWaitSemaphores    = std::data( present_signal_semaphores );
    PresentInfo.swapchainCount     = static_cast< uint32_t >( std::size( Swapchains ) );
    PresentInfo.pSwapchains        = std::data( Swapchains );
    PresentInfo.pImageIndices      = std::data( Img_Idxs );
    PresentInfo.pResults           = nullptr;

    auto Result = vkQueuePresentKHR( Ctx.PresentQueue, &PresentInfo );
    vkQueueWaitIdle( Ctx.PresentQueue );

    auto const change_Swapchain = ( Result == VK_ERROR_OUT_OF_DATE_KHR ) || ( Result == VK_SUBOPTIMAL_KHR );

    if ( change_Swapchain || Ctx.FramebufferResized )
    {
      Ctx.FramebufferResized = false;
      recreateAfterSwapchainChange( Ctx );
      return;
    }

    MVK_VERIFY( VK_SUCCESS == Result );

    Ctx.CurrentFrameIdx = ( Ctx.CurrentFrameIdx + 1 ) % Context::MaxFramesInFlight;
    nextBuffer( Ctx );
  }

}  // namespace mvk::engine
