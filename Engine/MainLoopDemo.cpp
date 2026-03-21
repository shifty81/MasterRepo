#include "Engine/Window/Window.h"
#include "Engine/Input/InputManager.h"
#include "Engine/Render/Renderer.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // Window setup
    Engine::Window::WindowConfig winCfg;
    winCfg.title = "Main Loop Demo";
    Engine::Window::Window window(winCfg);
    if (!window.Init()) {
        std::cerr << "Failed to initialize window!\n";
        return 1;
    }

    // Renderer setup
    Engine::Render::RenderConfig renderCfg;
    Engine::Render::Renderer renderer(renderCfg);
    if (!renderer.Init(window.GetWidth(), window.GetHeight())) {
        std::cerr << "Renderer failed to initialize!\n";
        return 1;
    }

    // Input setup
    Engine::Input::InputManager input;
    input.Init();
    input.BindAction(Engine::Input::InputAction::Jump, Engine::Input::InputDevice::Keyboard, 32, "Space");

    // Main loop
    while (!window.ShouldClose()) {
        window.PollEvents();
        renderer.BeginFrame();
        renderer.Clear();
        renderer.EndFrame();
        window.SwapBuffers();
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }

    input.Shutdown();
    renderer.Shutdown();
    window.Shutdown();
    std::cout << "Main loop exited cleanly." << std::endl;
    return 0;
}
