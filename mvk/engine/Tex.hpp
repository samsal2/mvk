#ifndef MVK_ENGINE_TEX_HPP_INCLUDED
#define MVK_ENGINE_TEX_HPP_INCLUDED

#include "Engine/Context.hpp"
#include "Engine/StagingBuff.hpp"

namespace Mvk::Engine {

// FIXME(samuel): This is not going anywhere
class Tex {
public:
  struct StageResult {
    VkDescriptorSet DescriptorSet;
  };

  static void transitionImgLayout(VkCommandBuffer CmdBuff, VkImage Img,
                                  VkImageLayout OldLay, VkImageLayout NewLay,
                                  uint32_t MipLvl) noexcept;

  static void generateMip(VkCommandBuffer CmdBuff, VkImage Img, uint32_t Width,
                          uint32_t Height, uint32_t MipLvl) noexcept;

  Tex(Context &Ctx, StagingBuff::MapResult Data, uint32_t Width,
      uint32_t Height) noexcept;

  Tex(Tex const &Other) noexcept = delete;
  Tex(Tex &&other) noexcept = delete;

  Tex &operator=(Tex const &Other) noexcept = delete;
  Tex &operator=(Tex &&Other) noexcept = delete;

  ~Tex() noexcept;

  // TODO(samuel): Name
  [[nodiscard]] StageResult stage() noexcept { return {DescriptorSet}; }
  [[nodiscard]] uint32_t size() const noexcept { return Width * Height * 4; }

private:
  enum class AllocState : int { Allocated, Deallocated };

  void allocate(StagingBuff::MapResult From) noexcept;
  void deallocate() noexcept;
  void write() noexcept;
  void stage(StagingBuff::MapResult From) noexcept;
  void generateMip() noexcept;

  AllocState State;
  Context &Ctx;
  uint32_t Width;
  uint32_t Height;
  uint32_t MipLvl;
  VkImage Img;
  VkImageView ImgView;
  VkDeviceMemory Mem;
  VkDescriptorSet DescriptorSet;
};

} // namespace Mvk::Engine

#endif
