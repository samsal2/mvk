#include "types/types.hpp"

#include "utility/verify.hpp"

#include <vector>

namespace mvk::types
{
  [[nodiscard]] types::unique_buffer create_unique_buffer( types::device const        device,
                                                           VkBufferCreateInfo const & info ) noexcept
  {
    auto handle = types::buffer();

    [[maybe_unused]] auto result = vkCreateBuffer( types::get( device ), &info, nullptr, &types::get( handle ) );

    MVK_VERIFY( result == VK_SUCCESS );

    return types::unique_buffer( types::get( handle ), types::get( device ) );
  }

  [[nodiscard]] types::unique_pipeline_layout
    create_unique_pipeline_layout( device const device, VkPipelineLayoutCreateInfo const & info ) noexcept
  {
    auto handle = types::pipeline_layout();

    [[maybe_unused]] auto result =
      vkCreatePipelineLayout( types::get( device ), &info, nullptr, &types::get( handle ) );

    MVK_VERIFY( result == VK_SUCCESS );

    return types::unique_pipeline_layout( types::get( handle ), types::get( device ) );
  }

  [[nodiscard]] std::vector< unique_command_buffer >
    allocate_unique_command_buffers( device const device, VkCommandBufferAllocateInfo const & info ) noexcept
  {
    auto const size    = info.commandBufferCount;
    auto       handles = std::vector< typename command_buffer::handle_type >( size );

    [[maybe_unused]] auto result = vkAllocateCommandBuffers( types::get( device ), &info, std::data( handles ) );

    MVK_VERIFY( result == VK_SUCCESS );

    auto command_buffers = std::vector< unique_command_buffer >();
    command_buffers.reserve( size );

    for ( auto const handle : handles )
    {
      auto const command_pool = info.commandPool;
      command_buffers.emplace_back( handle, types::get( device ), command_pool );
    }

    return command_buffers;
  }

  [[nodiscard]] std::vector< unique_descriptor_set >
    allocate_unique_descriptor_sets( device const device, VkDescriptorSetAllocateInfo const & info ) noexcept
  {
    auto const size    = info.descriptorSetCount;
    auto       handles = std::vector< typename descriptor_set::handle_type >( size );

    [[maybe_unused]] auto result = vkAllocateDescriptorSets( types::get( device ), &info, std::data( handles ) );

    MVK_VERIFY( result == VK_SUCCESS );

    auto descriptor_sets = std::vector< unique_descriptor_set >();
    descriptor_sets.reserve( size );

    for ( auto const handle : handles )
    {
      auto const command_pool = info.descriptorPool;
      descriptor_sets.emplace_back( handle, types::get( device ), command_pool );
    }

    return descriptor_sets;
  }

  [[nodiscard]] types::unique_command_pool create_unique_command_pool( device const                    device,
                                                                       VkCommandPoolCreateInfo const & info ) noexcept
  {
    auto                  handle = types::command_pool();
    [[maybe_unused]] auto result = vkCreateCommandPool( types::get( device ), &info, nullptr, &types::get( handle ) );
    return types::unique_command_pool( types::get( handle ), types::get( device ) );
  }

  [[nodiscard]] types::unique_descriptor_pool
    create_unique_descriptor_pool( device const device, VkDescriptorPoolCreateInfo const & info ) noexcept
  {
    auto handle = types::descriptor_pool();

    [[maybe_unused]] auto result =
      vkCreateDescriptorPool( types::get( device ), &info, nullptr, &types::get( handle ) );

    MVK_VERIFY( result == VK_SUCCESS );

    return types::unique_descriptor_pool( types::get( handle ), types::get( device ) );
  }

  [[nodiscard]] types::unique_descriptor_set_layout
    create_unique_descriptor_set_layout( device const device, VkDescriptorSetLayoutCreateInfo const & info ) noexcept
  {
    auto handle = types::descriptor_set_layout();

    [[maybe_unused]] auto result =
      vkCreateDescriptorSetLayout( types::get( device ), &info, nullptr, &types::get( handle ) );

    MVK_VERIFY( result == VK_SUCCESS );

    return types::unique_descriptor_set_layout( types::get( handle ), types::get( device ) );
  }

  [[nodiscard]] types::unique_device_memory create_unique_device_memory( device                       device,
                                                                         VkMemoryAllocateInfo const & info ) noexcept
  {
    auto handle = types::device_memory();

    [[maybe_unused]] auto result = vkAllocateMemory( types::get( device ), &info, nullptr, &types::get( handle ) );

    MVK_VERIFY( result == VK_SUCCESS );

    return types::unique_device_memory( types::get( handle ), types::get( device ) );
  }

  [[nodiscard]] queue get_queue( device device, queue_index index ) noexcept
  {
    auto handle = types::queue();
    vkGetDeviceQueue( types::get( device ), index, 0, &types::get( handle ) );
    return queue( handle );
  }

  [[nodiscard]] types::unique_device create_unique_device( physical_device const      physical_device,
                                                           VkDeviceCreateInfo const & info ) noexcept
  {
    auto handle = types::device();

    [[maybe_unused]] auto result =
      vkCreateDevice( types::get( physical_device ), &info, nullptr, &types::get( handle ) );

    MVK_VERIFY( result == VK_SUCCESS );
    return types::unique_device( types::get( handle ) );
  }

  [[nodiscard]] types::unique_fence create_unique_fence( device const device, VkFenceCreateInfo const & info ) noexcept
  {
    auto handle = types::fence();

    [[maybe_unused]] auto result = vkCreateFence( types::get( device ), &info, nullptr, &types::get( handle ) );

    MVK_VERIFY( result == VK_SUCCESS );

    return types::unique_fence( types::get( handle ), types::get( device ) );
  }

  [[nodiscard]] types::unique_framebuffer create_unique_framebuffer( device const                    device,
                                                                     VkFramebufferCreateInfo const & info ) noexcept
  {
    auto handle = types::framebuffer();

    [[maybe_unused]] auto result = vkCreateFramebuffer( types::get( device ), &info, nullptr, &types::get( handle ) );

    MVK_VERIFY( result == VK_SUCCESS );

    return types::unique_framebuffer( types::get( handle ), types::get( device ) );
  }

  [[nodiscard]] types::unique_image_view create_unique_image_view( device const                  device,
                                                                   VkImageViewCreateInfo const & info ) noexcept
  {
    auto handle = types::image_view();

    [[maybe_unused]] auto result = vkCreateImageView( types::get( device ), &info, nullptr, &types::get( handle ) );

    MVK_VERIFY( result == VK_SUCCESS );

    return types::unique_image_view( types::get( handle ), types::get( device ) );
  }

  [[nodiscard]] types::unique_instance create_unique_instance( VkInstanceCreateInfo const & info ) noexcept
  {
    auto handle = types::instance();

    [[maybe_unused]] auto result = vkCreateInstance( &info, nullptr, &types::get( handle ) );

    MVK_VERIFY( result == VK_SUCCESS );

    return types::unique_instance( types::get( handle ) );
  }

  [[nodiscard]] types::unique_pipeline create_unique_pipeline( device const                         device,
                                                               VkGraphicsPipelineCreateInfo const & info ) noexcept
  {
    auto handle = types::pipeline();

    [[maybe_unused]] auto result =
      vkCreateGraphicsPipelines( types::get( device ), VK_NULL_HANDLE, 1, &info, nullptr, &types::get( handle ) );

    MVK_VERIFY( result == VK_SUCCESS );

    return types::unique_pipeline( types::get( handle ), types::get( device ) );
  }

  [[nodiscard]] types::unique_render_pass create_unique_render_pass( device const                   device,
                                                                     VkRenderPassCreateInfo const & info ) noexcept
  {
    auto handle = types::render_pass();

    [[maybe_unused]] auto result = vkCreateRenderPass( types::get( device ), &info, nullptr, &types::get( handle ) );

    MVK_VERIFY( result == VK_SUCCESS );

    return types::unique_render_pass( types::get( handle ), types::get( device ) );
  }

  [[nodiscard]] types::unique_sampler create_unique_sampler( device const                device,
                                                             VkSamplerCreateInfo const & info ) noexcept
  {
    auto handle = types::sampler();

    [[maybe_unused]] auto result = vkCreateSampler( types::get( device ), &info, nullptr, &types::get( handle ) );

    MVK_VERIFY( result == VK_SUCCESS );

    return types::unique_sampler( types::get( handle ), types::get( device ) );
  }

  [[nodiscard]] types::unique_semaphore create_unique_semaphore( device const                  device,
                                                                 VkSemaphoreCreateInfo const & info ) noexcept
  {
    auto handle = types::semaphore();

    [[maybe_unused]] auto result = vkCreateSemaphore( types::get( device ), &info, nullptr, &types::get( handle ) );

    MVK_VERIFY( result == VK_SUCCESS );

    return types::unique_semaphore( types::get( handle ), types::get( device ) );
  }

  [[nodiscard]] types::unique_image create_unique_image( device const device, VkImageCreateInfo const & info ) noexcept
  {
    auto handle = types::image();

    [[maybe_unused]] auto result = vkCreateImage( types::get( device ), &info, nullptr, &types::get( handle ) );

    MVK_VERIFY( result == VK_SUCCESS );

    return types::unique_image( types::get( handle ), types::get( device ) );
  }

  [[nodiscard]] types::unique_shader_module
    create_unique_shader_module( device const device, VkShaderModuleCreateInfo const & info ) noexcept
  {
    auto handle = types::shader_module();

    [[maybe_unused]] auto result = vkCreateShaderModule( types::get( device ), &info, nullptr, &types::get( handle ) );

    MVK_VERIFY( result == VK_SUCCESS );

    return types::unique_shader_module( types::get( handle ), types::get( device ) );
  }

  [[nodiscard]] types::unique_swapchain create_unique_swapchain( device const                     device,
                                                                 VkSwapchainCreateInfoKHR const & info ) noexcept
  {
    auto handle = types::swapchain();

    [[maybe_unused]] auto result = vkCreateSwapchainKHR( types::get( device ), &info, nullptr, &types::get( handle ) );

    MVK_VERIFY( result == VK_SUCCESS );

    return types::unique_swapchain( types::get( handle ), types::get( device ) );
  }

}  // namespace mvk::types
