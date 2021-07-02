#ifndef MVK_RENDERER_HPP_INCLUDED
#define MVK_RENDERER_HPP_INCLUDED

#include "detail/helpers.hpp"
#include "shader_types.hpp"
#include "types/types.hpp"

namespace mvk::detail
{
  static VKAPI_ATTR VKAPI_CALL VkBool32
    debug_callback( [[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT       severity,
                    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT              type,
                    [[maybe_unused]] VkDebugUtilsMessengerCallbackDataEXT const * data,
                    [[maybe_unused]] void *                                       p_user_data )
  {
    std::cerr << data->pMessage << '\n';
    return VK_FALSE;
  }

  static constexpr auto debug_create_info = []
  {
    auto tmp            = VkDebugUtilsMessengerCreateInfoEXT();
    tmp.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    tmp.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |  // NOLINT
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    tmp.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |     // NOLINT
                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |  // NO
                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    tmp.pfnUserCallback = debug_callback;
    tmp.pUserData       = nullptr;
    return tmp;
  }();

  [[nodiscard]] constexpr auto aligned_size( utility::integral auto size, utility::integral auto alignment ) noexcept
  {
    if ( auto mod = size % alignment; mod != 0 )
    {
      return size + alignment - mod;
    }

    return static_cast<decltype( size + alignment )>( size );
  }

}  // namespace mvk::detail

namespace mvk
{
  struct context
  {
    void basic_draw( utility::slice<std::byte const> vertices,
                     utility::slice<std::byte const> indices,
                     utility::slice<std::byte const> pvm );

    static constexpr auto use_validation                 = true;
    static constexpr auto validation_layers              = std::array{ "VK_LAYER_KHRONOS_validation" };
    static constexpr auto validation_instance_extensions = std::array{ VK_EXT_DEBUG_UTILS_EXTENSION_NAME };

    static constexpr auto device_extensions = std::array{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    static constexpr auto   max_frames_in_flight = 2;
    static constexpr size_t dynamic_buffer_count = 2;
    static constexpr size_t garbage_buffer_count = 2;

    // init_window
    types::window window_;

    // init_instance
    types::unique_instance instance_;

    // init_surface
    types::unique_surface surface_;

    // init_debug_messenger
    types::unique_debug_messenger debug_messenger_;

    // init_device
    types::physical_device physical_device_;
    types::unique_device   device_;
    u32                    graphics_queue_index_;
    u32                    present_queue_index_;
    types::queue           graphics_queue_;
    types::queue           present_queue_;

    // init_layouts
    types::unique_descriptor_set_layout main_descriptor_set_layout_;
    types::unique_pipeline_layout       pipeline_layout_;

    // init_pools
    types::unique_command_pool    command_pool_;
    types::unique_descriptor_pool descriptor_pool_;

    // init_swapchain
    u32                                   swapchain_images_count_;
    types::unique_swapchain               swapchain_;
    std::vector<types::unique_image_view> swapchain_image_views_;
    VkExtent2D                            swapchain_extent_;

    // init_depth_image
    types::unique_image         depth_image_;
    types::unique_device_memory depth_image_memory_;
    types::unique_image_view    depth_image_view_;

    // init_framebuffers
    std::vector<types::unique_framebuffer> framebuffers_;

    // init_main_renderpass
    types::unique_render_pass render_pass_;

    // doesnt belong here
    // ================================================================================================================
    std::vector<unsigned char>  texture_;
    u32                         width_;
    u32                         height_;
    types::unique_image         image_;
    types::unique_device_memory image_memory_;
    types::unique_image_view    image_view_;
    types::unique_sampler       sampler_;
    std::vector<vertex>         vertices_;
    std::vector<u32>            indices_;
    // ================================================================================================================

    // allocate_commands
    std::vector<types::unique_command_buffer> command_buffers_;

    // init_descriptors
    std::vector<types::unique_descriptor_set> descriptor_sets_;

    // init_ubos
    std::vector<types::unique_buffer>        uniform_buffers_;
    std::vector<types::unique_device_memory> uniform_buffers_memory_;
    std::vector<utility::slice<std::byte>>   mapped_datas_;

    // shaders
    types::unique_shader_module vertex_shader_;
    types::unique_shader_module fragment_shader_;

    // init_pipeline
    types::unique_pipeline pipeline_;

    // init_sync
    std::array<types::unique_semaphore, max_frames_in_flight> image_available_semaphores_;
    std::array<types::unique_semaphore, max_frames_in_flight> render_finished_semaphores_;
    std::array<types::unique_fence, max_frames_in_flight>     frame_in_flight_fences_;
    std::vector<types::unique_fence *>                        image_in_flight_fences_;

    // buffers
    // vertex
    VkMemoryRequirements                                   vertex_memory_requirements_;
    types::device_size                                     vertex_aligned_size_;
    std::array<types::unique_buffer, dynamic_buffer_count> vertex_buffers_;
    std::array<types::device_size, dynamic_buffer_count>   vertex_offsets_;
    types::unique_device_memory                            vertex_memory_;

    // index
    VkMemoryRequirements                                   index_memory_requirements_;
    types::device_size                                     index_aligned_size_;
    std::array<types::unique_buffer, dynamic_buffer_count> index_buffers_;
    std::array<types::device_size, dynamic_buffer_count>   index_offsets_;
    types::unique_device_memory                            index_memory_;

    // staging
    VkMemoryRequirements                                   staging_memory_requirements_;
    types::device_size                                     staging_aligned_size_;
    std::array<types::unique_buffer, dynamic_buffer_count> staging_buffers_;
    std::array<types::device_size, dynamic_buffer_count>   staging_offsets_;
    types::unique_device_memory                            staging_memory_;
    utility::slice<std::byte>                              staging_data_;

    // garbage
    std::array<std::vector<types::unique_buffer>, garbage_buffer_count>        garbage_buffers_;
    std::array<std::vector<types::unique_device_memory>, garbage_buffer_count> garbage_memories_;

    // rendering counters
    size_t                current_frame_index_    = 0;
    size_t                current_buffer_index_   = 0;
    size_t                current_garbage_index_  = 0;
    u32                   current_image_index_    = 0;
    types::command_buffer current_command_buffer_ = VK_NULL_HANDLE;

    std::chrono::time_point<std::chrono::high_resolution_clock> start_time = std::chrono::high_resolution_clock::now();
  };

  [[nodiscard]] context create_context( std::string const & name, types::window::extent extent ) noexcept;

  void recreate_after_swapchain_change( context & ctx ) noexcept;
  void begin_draw( context & ctx ) noexcept;

  void basic_draw( context &                       ctx,
                   utility::slice<std::byte const> vertices,
                   utility::slice<std::byte const> indices,
                   utility::slice<std::byte const> pvm ) noexcept;

  void end_draw( context & ctx ) noexcept;

  [[nodiscard]] float current_time( context const & ctx ) noexcept;

  [[nodiscard]] pvm create_test_pvm( context const & ctx ) noexcept;

  void test_run( context & ctx ) noexcept;

  // init_
  void init_window( context & ctx, types::window::extent extent ) noexcept;
  void init_instance( context & ctx, std::string const & name ) noexcept;
  void init_debug_messenger( context & ctx ) noexcept;
  void init_surface( context & ctx ) noexcept;
  void init_device( context & ctx ) noexcept;
  void init_layouts( context & ctx ) noexcept;
  void init_pools( context & ctx ) noexcept;
  void init_swapchain( context & ctx ) noexcept;
  void init_depth_image( context & ctx ) noexcept;
  void init_framebuffers( context & ctx ) noexcept;
  void init_main_render_pass( context & ctx ) noexcept;
  void init_doesnt_belong_here( context & ctx ) noexcept;
  void allocate_command_buffers( context & ctx ) noexcept;
  void allocate_descriptors( context & ctx ) noexcept;
  void init_ubos( context & ctx ) noexcept;
  void init_shaders( context & ctx ) noexcept;
  void init_pipeline( context & ctx ) noexcept;
  void init_sync( context & ctx ) noexcept;

  // misc
  [[nodiscard]] types::unique_command_buffer allocate_single_use_command_buffer( context const & ctx ) noexcept;

  void transition_layout( context const & ctx,
                          types::image    image,
                          VkImageLayout   old_layout,
                          VkImageLayout   new_layout,
                          u32             mipmap_levels ) noexcept;

  void generate_mipmaps( context const & ctx, types::image image, u32 width, u32 height, u32 mipmap_levels ) noexcept;

  // buffers
  void create_vertex_buffers_and_memories( context & ctx, types::device_size size ) noexcept;
  void create_index_buffers_and_memories( context & ctx, types::device_size size ) noexcept;
  void create_staging_buffers_and_memories( context & ctx, types::device_size size ) noexcept;

  void move_to_garbage_buffers( context &                                                           ctx,
                                utility::slice<types::unique_buffer, context::dynamic_buffer_count> buffers ) noexcept;

  void move_to_garbage_memories( context & ctx, types::unique_device_memory memory ) noexcept;

  struct staging_allocation
  {
    types::buffer      buffer_;
    types::device_size offset_;
    types::device_size size_;
  };

  [[nodiscard]] staging_allocation staging_allocate( context & ctx, utility::slice<std::byte const> src ) noexcept;
  void stage_image( context & ctx, staging_allocation allocation, u32 width, u32 height, types::image image ) noexcept;

  struct vertex_allocation
  {
    types::buffer      buffer_;
    types::device_size offset_;
  };

  [[nodiscard]] vertex_allocation vertex_allocate( context & ctx, staging_allocation allocation ) noexcept;

  struct index_allocation
  {
    types::buffer      buffer_;
    types::device_size offset_;
  };

  [[nodiscard]] index_allocation index_allocate( context & ctx, staging_allocation allocation ) noexcept;

  void next_buffer( context & ctx ) noexcept;

}  // namespace mvk

#endif
