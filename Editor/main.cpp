#include "Editor/Render/EditorRenderer.h"
#include "Engine/Window/Window.h"
#include "Engine/Core/Logger.h"
#include "Editor/UI/EditorLayout.h"
#include "Editor/Panels/Console/ConsolePanel.h"
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

    renderer.AppendConsole("[Info]  All editor systems online");

    Engine::Core::Logger::Info("AtlasEditor running — press ESC to quit");

    // ── Main render loop ─────────────────────────────────────────────────────
    double prevTime = glfwGetTime();
    while (!window.ShouldClose()) {
        double now = glfwGetTime();
        double dt  = now - prevTime;
        prevTime   = now;

        window.PollEvents();
        renderer.Render(dt);
        window.SwapBuffers();
    }

    renderer.Shutdown();
    window.Shutdown();
    Engine::Core::Logger::Info("AtlasEditor shutdown");
    return 0;
}

