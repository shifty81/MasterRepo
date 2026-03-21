#include "Engine/Window/Window.h"
#include "Engine/Input/InputManager.h"
#include "Engine/Render/Renderer.h"
#include "Editor/Core/EditorShell.h"
#include <iostream>
#include <thread>
#include <chrono>

struct DummyPanel : public Editor::Panel {
    DummyPanel(const std::string& n) { name = n; }
    void Draw() override { std::cout << "Drawing panel: " << name << std::endl; }
};

int main() {
    // Window setup
    Engine::Window::WindowConfig winCfg;
    winCfg.title = "Editor Prototype";
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

    // Editor shell setup
    Editor::EditorShell shell;
    shell.AddPanel(std::make_unique<Editor::ViewportPanel>());
    shell.AddPanel(std::make_unique<Editor::InspectorPanel>());
    shell.AddPanel(std::make_unique<DummyPanel>("Hierarchy"));

    // Main loop
    while (!window.ShouldClose()) {
        window.PollEvents();
        renderer.BeginFrame();
        renderer.Clear();
        shell.Draw(); // Simulate drawing all panels
        renderer.EndFrame();
        window.SwapBuffers();
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }

    input.Shutdown();
    renderer.Shutdown();
    window.Shutdown();
    std::cout << "Editor prototype exited cleanly." << std::endl;
    return 0;
}
