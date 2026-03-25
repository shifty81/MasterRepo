#include "Editor/Render/EditorRenderer.h"
#include "Engine/Window/Window.h"
#include "Engine/Core/Logger.h"
#include "Editor/UI/EditorLayout.h"
#include "Editor/Panels/Console/ConsolePanel.h"
#include "Runtime/ECS/ECS.h"
#include "Runtime/Components/Components.h"
#include <iostream>
#include <filesystem>
#include <GLFW/glfw3.h>  // for glfwGetTime

int main() {
    std::cout << "[Editor] Working directory: "
              << std::filesystem::current_path().string() << "\n";

    Engine::Core::Logger::Init();

    // ── Window ──────────────────────────────────────────────────────────────
    Engine::Window::WindowConfig winCfg;
    winCfg.title     = "AtlasEditor  v0.1";
    winCfg.width     = 1280;
    winCfg.height    = 720;
    winCfg.vsync     = true;
    winCfg.resizable = true;

    Engine::Window::Window window(winCfg);

    // ── Renderer ─────────────────────────────────────────────────────────────
    Editor::EditorRenderer renderer;

    // Wire up callbacks before Init so they're active from the first frame
    window.onMouseMove   = [&](double x, double y) { renderer.OnMouseMove(x, y); };
    window.onMouseButton = [&](int btn, bool pressed) { renderer.OnMouseButton(btn, pressed); };
    window.onKey         = [&](int key, bool pressed) {
        renderer.OnKey(key, pressed);
        // ESC closes the editor
        if (key == GLFW_KEY_ESCAPE && pressed)
            glfwSetWindowShouldClose(window.GetGLFWHandle(), GLFW_TRUE);
    };
    window.onResize = [&](int w, int h) { renderer.Resize(w, h); };

    if (!window.Init()) {
        std::cerr << "[Editor] Failed to open window\n";
        return 1;
    }

    if (!renderer.Init(winCfg.width, winCfg.height)) {
        std::cerr << "[Editor] Failed to init renderer\n";
        return 1;
    }

    // EI-01: Register logger sink so every log line appears in the console panel
    Engine::Core::Logger::SetSink([&](const std::string& msg) {
        renderer.AppendConsole(msg);
    });

    renderer.AppendConsole("[Info]  All editor systems online");

    // EI-02: Create a live ECS world and populate it with default entities
    Runtime::ECS::World world;

    // PlayerShip entity
    {
        auto id = world.CreateEntity();
        world.AddComponent(id, Runtime::Components::Tag{"PlayerShip", {"Player", "Ship"}});
        Runtime::Components::Transform tr;
        tr.position = {0.f, 0.f, 0.f};
        world.AddComponent(id, tr);
        Runtime::Components::MeshRenderer mr;
        mr.meshId = "player_ship.obj"; mr.materialId = "ship_mat"; mr.visible = true;
        world.AddComponent(id, mr);
    }
    // Station Hull
    {
        auto id = world.CreateEntity();
        world.AddComponent(id, Runtime::Components::Tag{"Station_Hull", {"Structure","Snappable"}});
        Runtime::Components::Transform tr; tr.position = {3.f, 0.f, 0.f};
        world.AddComponent(id, tr);
        Runtime::Components::MeshRenderer mr; mr.meshId = "hull_v2.obj";
        world.AddComponent(id, mr);
        Runtime::Components::RigidBody rb; rb.mass = 1200.f; rb.isKinematic = false;
        world.AddComponent(id, rb);
    }
    // Engine_L
    {
        auto id = world.CreateEntity();
        world.AddComponent(id, Runtime::Components::Tag{"Engine_L", {"Engine"}});
        Runtime::Components::Transform tr; tr.position = {-2.f, -1.f, 0.f};
        world.AddComponent(id, tr);
    }
    // Engine_R
    {
        auto id = world.CreateEntity();
        world.AddComponent(id, Runtime::Components::Tag{"Engine_R", {"Engine"}});
        Runtime::Components::Transform tr; tr.position = {2.f, -1.f, 0.f};
        world.AddComponent(id, tr);
    }
    // Turret_01
    {
        auto id = world.CreateEntity();
        world.AddComponent(id, Runtime::Components::Tag{"Turret_01", {"Weapon"}});
        Runtime::Components::Transform tr; tr.position = {-1.f, 2.f, 0.f};
        world.AddComponent(id, tr);
    }
    // AsteroidCluster
    {
        auto id = world.CreateEntity();
        world.AddComponent(id, Runtime::Components::Tag{"AsteroidCluster", {"PCG", "Debris"}});
        Runtime::Components::Transform tr; tr.position = {6.f, 3.f, 0.f};
        world.AddComponent(id, tr);
    }

    renderer.SetWorld(&world);

    Engine::Core::Logger::Info("AtlasEditor running — press ESC to quit");

    // ── Main render loop ─────────────────────────────────────────────────────
    double prevTime = glfwGetTime();
    while (!window.ShouldClose()) {
        double now = glfwGetTime();
        double dt  = now - prevTime;
        prevTime   = now;

        window.PollEvents();

        // EI-13: tick the ECS world when PIE is active (world.Update ticks systems)
        // world.Update((float)dt);  // enable when PIE is wired to a sub-context

        renderer.Render(dt);
        window.SwapBuffers();
    }

    renderer.Shutdown();
    window.Shutdown();
    Engine::Core::Logger::Info("AtlasEditor shutdown");
    return 0;
}

