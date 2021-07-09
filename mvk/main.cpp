#include "Engine/Misc.hpp"
#include "Engine/Model.hpp"
#include "Engine/VulkanRenderer.hpp"
#include "GLFW/glfw3.h"
#include "vulkan/vulkan_core.h"

#include <iostream>

int main()
{
  auto Rdr = Mvk::Engine::VulkanRenderer();

  auto ID = Rdr.loadModel();

  while ( !Rdr.isDone() )
  {
    glfwPollEvents();

    Rdr.beginDraw();

    Rdr.drawModel( ID );

    Rdr.endDraw();
  }

  return 0;
}
