#include "Engine/Render/Renderer.h"
#include <iostream>

int main() {
    Engine::Render::RenderConfig cfg;
    Engine::Render::Renderer renderer(cfg);
    if (!renderer.Init(800, 600)) {
        std::cerr << "Renderer failed to initialize!\n";
        return 1;
    }
    renderer.SetClearColor(0.1f, 0.2f, 0.3f);
    renderer.BeginFrame();
    renderer.Clear();
    renderer.EndFrame();
    renderer.Shutdown();
    std::cout << "Renderer demo complete.\n";
    return 0;
}