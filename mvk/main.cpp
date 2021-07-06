#include "Engine/Misc.hpp"
#include "Engine/Renderer.hpp"

int main() {
  auto Render = Mvk::Engine::Renderer(std::string("stan loona"), {600, 600});
  Mvk::Engine::testRun(Render);

  return 0;
}
