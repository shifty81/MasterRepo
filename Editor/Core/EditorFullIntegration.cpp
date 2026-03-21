#include "Engine/Window/Window.h"
#include "Engine/Input/InputManager.h"
#include "Engine/Render/Renderer.h"
#include "Engine/Scene/SceneGraph.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Audio/AudioEngine.h"
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
    winCfg.title = "Editor Full Integration";
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

    // Scene/camera setup
    Engine::SceneNode root{"Root"};
    root.children.push_back({"Cube", {0,0,0}});
    Engine::Camera camera;

    // Physics setup
    Engine::Physics::PhysicsWorld physics;
    physics.Init();
    auto bodyId = physics.CreateBody(1.0f);
    physics.SetPosition(bodyId, 0, 0, 0);
    physics.SetVelocity(bodyId, 0, 1, 0);

    // Audio setup
    Engine::Audio::AudioEngine audio;
    audio.Init();
    auto soundId = audio.LoadSound("test_sound.wav");

    // Editor shell setup
    Editor::EditorShell shell;
    shell.AddPanel(std::make_unique<Editor::ViewportPanel>());
    shell.AddPanel(std::make_unique<Editor::InspectorPanel>());
    shell.AddPanel(std::make_unique<DummyPanel>("Hierarchy"));

    // Main loop
    int frame = 0;
    while (!window.ShouldClose() && frame < 120) { // Run for 2 seconds
        window.PollEvents();
        renderer.BeginFrame();
        renderer.Clear();
        shell.Draw(); // Simulate drawing all panels
        // Physics step
        physics.Step(1.0f/60.0f);
        auto* body = physics.GetBody(bodyId);
        if (body && frame % 30 == 0) {
            std::cout << "Body position: (" << body->position.x << ", " << body->position.y << ", " << body->position.z << ")\n";
            if (audio.HasSound(soundId)) audio.Play(soundId);
        }
        renderer.EndFrame();
        window.SwapBuffers();
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        ++frame;
    }

    input.Shutdown();
    renderer.Shutdown();
    window.Shutdown();
    physics.Shutdown();
    audio.Shutdown();
    std::cout << "Editor full integration exited cleanly." << std::endl;
    return 0;
}
