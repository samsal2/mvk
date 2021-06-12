#ifndef MVK_TYPES_IMAGE_HPP_INCLUDE
#define MVK_TYPES_IMAGE_HPP_INCLUDE

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"

#include <filesystem>

namespace mvk::types
{

class device;
class command_pool;

class image : public detail::wrapper<detail::deleter<vkDestroyImage>,
                                     detail::handle<VkImage>,
                                     detail::parent<VkDevice>>
{
public:
  constexpr image() noexcept = default;

  image(VkDevice device, VkImageCreateInfo const & info);

  class texture
  {
  public:
    using value_type = unsigned char;

    constexpr texture() noexcept = default;
    explicit texture(std::filesystem::path const & path);

    [[nodiscard]] constexpr value_type const *
    data() const noexcept;

    [[nodiscard]] constexpr uint32_t
    width() const noexcept;

    [[nodiscard]] constexpr uint32_t
    height() const noexcept;

    [[nodiscard]] constexpr uint32_t
    size() const noexcept;

    [[nodiscard]] constexpr uint32_t
    mipmap_levels() const noexcept;

  private:
    struct deleter
    {
      void
      operator()(value_type * pixels) const noexcept;
    };

    std::unique_ptr<value_type, deleter> pixels_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t channels_ = 0;
    uint32_t mipmap_levels_ = 0;
  };

  image &
  transition_layout(device const & device, command_pool const & command_pool,
                    VkImageLayout old_layout, VkImageLayout new_layout);

  image &
  stage(device const & device, command_pool const & command_pool,
        image::texture const & texture);

  // TODO(samuel): current layout needs to be VK_IMAGE_LAYOUT_DST_OPTIMAL
  // and generate mipmaps transitions it to
  // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
  //
  // make it starting layout agnostic
  image &
  generate_mipmaps(device const & device, command_pool const & command_pool,
                   uint32_t width, uint32_t height);

  [[nodiscard]] constexpr VkMemoryRequirements
  memory_requirements() const noexcept;

  [[nodiscard]] constexpr uint32_t
  mipmap_levels() const noexcept;

private:
  uint32_t mipmap_levels_ = 0;
  VkMemoryRequirements memory_requirements_ = {};
};

[[nodiscard]] constexpr typename image::texture::value_type const *
image::texture::data() const noexcept
{
  return pixels_.get();
}

[[nodiscard]] constexpr uint32_t
image::texture::width() const noexcept
{
  return width_;
}

[[nodiscard]] constexpr uint32_t
image::texture::height() const noexcept
{
  return height_;
}

[[nodiscard]] constexpr uint32_t
image::texture::size() const noexcept
{
  // FIXME(samuel): hardcoded :(
  return width_ * height_ * 4 * sizeof(value_type);
}

[[nodiscard]] constexpr uint32_t
image::texture::mipmap_levels() const noexcept
{
  return mipmap_levels_;
}

[[nodiscard]] constexpr VkMemoryRequirements
image::memory_requirements() const noexcept
{
  return memory_requirements_;
}

[[nodiscard]] constexpr uint32_t
image::mipmap_levels() const noexcept
{
  return mipmap_levels_;
}

} // namespace mvk::types
#endif
