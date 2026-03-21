#include "Engine/Window/Window.h"
#include <iostream>

int main() {
    Engine::Window::WindowConfig cfg;
    cfg.title = "Test Window";
    Engine::Window::Window window(cfg);
    if (!window.Init()) {
        std::cerr << "Failed to initialize window!\n";
        return 1;
    }
    std::cout << "Window created: " << cfg.title << " (" << window.GetWidth() << "x" << window.GetHeight() << ")\n";
    window.Shutdown();
    return 0;
}