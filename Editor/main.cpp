#include "Editor/Render/EditorRenderer.h"
#include "Engine/Window/Window.h"
#include "Engine/Core/Logger.h"
#include "Editor/UI/EditorLayout.h"
#include "Editor/Panels/Console/ConsolePanel.h"
#include "Runtime/ECS/ECS.h"
#include "Runtime/Components/Components.h"
#include <iostream>
#include <filesystem>
#include <GLFW/glfw3.h>  // for glfwGetTime; also brings in gl.h on all platforms

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
        // ESC is fully handled inside OnKey (stops PIE, closes panels).
        // It never closes the editor window — use File → Exit for that.
    };
    window.onChar   = [&](unsigned int cp) { renderer.OnChar(cp); };
    window.onScroll = [&](double dx, double dy) { renderer.OnScroll(dx, dy); };
    window.onResize = [&](int w, int h) { renderer.Resize(w, h); };

    if (!window.Init()) {
        std::cerr << "[Editor] Failed to open window\n";
        return 1;
    }

    // ── PL-05: Animated splash screen ──────────────────────────────────────
    // Show a brief loading splash while the engine initializes.
    // The splash is rendered via direct GL calls before EditorRenderer starts.
    {
        double splashStart = glfwGetTime();
        double splashDuration = 2.5; // seconds
        while (glfwGetTime() - splashStart < splashDuration && !window.ShouldClose()) {
            window.PollEvents();
            double t = glfwGetTime() - splashStart;
            float progress = (float)(t / splashDuration);

            glViewport(0, 0, winCfg.width, winCfg.height);
            glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // Draw a simple progress bar via GL quads
            glMatrixMode(GL_PROJECTION); glLoadIdentity();
            glOrtho(0.0, winCfg.width, winCfg.height, 0.0, -1.0, 1.0);
            glMatrixMode(GL_MODELVIEW);  glLoadIdentity();

            float bx = winCfg.width  * 0.2f;
            float by = winCfg.height * 0.56f;
            float bw = winCfg.width  * 0.6f;
            float bh = 8.f;

            // Background bar
            glColor4f(0.15f, 0.15f, 0.20f, 1.f);
            glBegin(GL_QUADS);
            glVertex2f(bx, by); glVertex2f(bx + bw, by);
            glVertex2f(bx + bw, by + bh); glVertex2f(bx, by + bh);
            glEnd();

            // Progress fill (blue → cyan wave)
            float r = 0.0f + 0.1f * progress;
            float g = 0.5f * progress;
            float b = 0.85f;
            glColor4f(r, g, b, 1.f);
            glBegin(GL_QUADS);
            glVertex2f(bx, by); glVertex2f(bx + bw * progress, by);
            glVertex2f(bx + bw * progress, by + bh); glVertex2f(bx, by + bh);
            glEnd();

            window.SwapBuffers();
        }
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

    // ── ECS World — First Solar System Scene ─────────────────────────────────
    // Layout (2-D top-down, 1 world-unit ≈ 20 px at default zoom):
    //   Star (Sol) at origin; planets orbit at increasing radii;
    //   asteroid belts between Mars/Jupiter and beyond Neptune;
    //   stations at strategic jump points; player ship + character near station.
    Runtime::ECS::World world;

    auto addEntity = [&](const char* name,
                         std::initializer_list<const char*> tags,
                         float x, float y, float z,
                         const char* mesh   = nullptr,
                         const char* mat    = nullptr,
                         bool hasMesh       = false,
                         float mass         = 0.f,
                         bool kinematic     = true) -> uint32_t
    {
        auto id = world.CreateEntity();
        Runtime::Components::Tag t;
        t.name = name;
        for (auto* tag : tags) t.tags.push_back(tag);
        world.AddComponent(id, t);
        Runtime::Components::Transform tr;
        tr.position = {x, y, z};
        world.AddComponent(id, tr);
        if (hasMesh) {
            Runtime::Components::MeshRenderer mr;
            mr.meshId     = mesh  ? mesh  : "";
            mr.materialId = mat   ? mat   : "";
            mr.visible    = true;
            world.AddComponent(id, mr);
        }
        if (mass > 0.f) {
            Runtime::Components::RigidBody rb;
            rb.mass       = mass;
            rb.isKinematic = kinematic;
            world.AddComponent(id, rb);
        }
        return id;
    };

    // ── Galaxy backdrop ──────────────────────────────────────────────────────
    addEntity("Galaxy_MilkyWay", {"Galaxy","Background"},
              0.f, 0.f, -1000.f,
              "galaxy_skybox.obj", "galaxy_mat", true);

    // ── Star: Sol ────────────────────────────────────────────────────────────
    addEntity("Sol", {"Star","Light","CelestialBody"},
              0.f, 0.f, 0.f,
              "star_sphere.obj", "star_mat_sol", true,
              1.989e6f, true);

    // ── Inner Planets ────────────────────────────────────────────────────────
    addEntity("Mercury", {"Planet","Rocky","CelestialBody"},
               4.f, 0.f, 0.f,
               "planet_sphere.obj", "planet_mat_mercury", true,
               3.3e2f, true);

    addEntity("Venus",   {"Planet","Rocky","CelestialBody"},
               7.f, 0.f, 0.f,
               "planet_sphere.obj", "planet_mat_venus", true,
               4.87e3f, true);

    addEntity("Earth",   {"Planet","Rocky","Habitable","CelestialBody"},
              11.f, 0.f, 0.f,
              "planet_sphere.obj", "planet_mat_earth", true,
              5.97e3f, true);

    addEntity("Luna",    {"Moon","Rocky","CelestialBody"},
              12.f, 0.f, 0.f,
              "moon_sphere.obj",   "planet_mat_luna",  true,
              7.35e1f, true);

    addEntity("Mars",    {"Planet","Rocky","CelestialBody"},
              15.f, 0.f, 0.f,
              "planet_sphere.obj", "planet_mat_mars",  true,
              6.39e2f, true);

    // ── Asteroid Belt Alpha (between Mars and Jupiter) ───────────────────────
    addEntity("AsteroidBelt_Alpha", {"AsteroidBelt","PCG","Debris"},
              20.f, 0.f, 0.f);
    {
        // Scatter a handful of named asteroids for testing
        static const char* aNames[] = {
            "Ceres","Vesta","Pallas","Hygiea","Interamnia"
        };
        static const float aOff[][2] = {
            {19.f,-1.5f},{20.5f,2.f},{18.5f,1.f},{21.f,-0.5f},{19.5f,2.5f}
        };
        for (int i = 0; i < 5; i++) {
            addEntity(aNames[i], {"Asteroid","Debris"},
                      aOff[i][0], aOff[i][1], 0.f,
                      "asteroid.obj", "asteroid_mat", true,
                      5.f + i * 2.f, false);
        }
    }

    // ── Outer Planets ────────────────────────────────────────────────────────
    addEntity("Jupiter", {"Planet","GasGiant","CelestialBody"},
              28.f, 0.f, 0.f,
              "planet_sphere.obj", "planet_mat_jupiter", true,
              1.898e6f, true);

    addEntity("Saturn",  {"Planet","GasGiant","CelestialBody"},
              38.f, 0.f, 0.f,
              "planet_sphere.obj", "planet_mat_saturn",  true,
              5.68e5f, true);

    addEntity("Uranus",  {"Planet","IceGiant","CelestialBody"},
              48.f, 0.f, 0.f,
              "planet_sphere.obj", "planet_mat_uranus",  true,
              8.68e4f, true);

    addEntity("Neptune", {"Planet","IceGiant","CelestialBody"},
              57.f, 0.f, 0.f,
              "planet_sphere.obj", "planet_mat_neptune", true,
              1.02e5f, true);

    // ── Kuiper Belt (outer debris field) ─────────────────────────────────────
    addEntity("KuiperBelt", {"AsteroidBelt","PCG","Debris"},
              65.f, 0.f, 0.f);
    addEntity("Pluto",   {"DwarfPlanet","KBO"},
              67.f, 2.f, 0.f,
              "asteroid.obj","planet_mat_pluto",true, 1.3e1f, true);
    addEntity("Eris",    {"DwarfPlanet","KBO"},
              69.f,-1.5f,0.f,
              "asteroid.obj","planet_mat_pluto",true, 1.66e1f, true);

    // ── Space Stations ───────────────────────────────────────────────────────
    // Orbital Station near Earth — L2 point
    addEntity("EarthStation_Athena",
              {"Station","Structure","SpaceStation","Dockable"},
              11.5f, 1.f, 0.f,
              "station_hull.obj", "station_mat", true,
              12000.f, false);

    // Deep-space refuelling outpost near Jupiter
    addEntity("JupiterOutpost_Forge",
              {"Station","Structure","SpaceStation","Dockable","Refuel"},
              29.f, 2.f, 0.f,
              "station_hull.obj", "station_mat_deep", true,
              8500.f, false);

    // Jump gate at system edge
    addEntity("JumpGate_Helios",
              {"JumpGate","Structure","Warp"},
              72.f, 0.f, 0.f,
              "jumpgate.obj", "gate_mat", true,
              50000.f, true);

    // ── Player Character ─────────────────────────────────────────────────────
    addEntity("Player_Avatar",
              {"Player","Character","Humanoid"},
              11.5f, 1.2f, 0.f,
              "character_player.obj", "character_mat", true,
              80.f, false);

    // ── Player Ship ──────────────────────────────────────────────────────────
    auto playerShipId =
    addEntity("NovaFighter_Mk1",
              {"PlayerShip","Ship","Fighter"},
              11.5f, 0.8f, 0.f,
              "nova_fighter.obj", "ship_mat_nova", true,
              2400.f, false);
    // Attach sub-components to player ship
    addEntity("Engine_L",  {"Engine","ShipPart"}, 11.1f, 0.6f, 0.f,
              nullptr, nullptr, false);
    addEntity("Engine_R",  {"Engine","ShipPart"}, 11.9f, 0.6f, 0.f,
              nullptr, nullptr, false);
    addEntity("Turret_Fwd",{"Weapon","ShipPart"},  11.5f, 1.1f, 0.f,
              nullptr, nullptr, false);
    addEntity("Shield_Gen",{"Shield","ShipPart"},  11.5f, 0.8f, 0.f,
              nullptr, nullptr, false);
    (void)playerShipId;

    // ── NPC Ships ────────────────────────────────────────────────────────────
    addEntity("NPC_Freighter_01",
              {"NPC","Ship","Freighter"},
              12.f, -1.f, 0.f,
              "freighter.obj", "ship_mat_freighter", true,
              15000.f, false);

    addEntity("NPC_Patrol_01",
              {"NPC","Ship","Fighter","Faction_Guard"},
              10.5f, 1.5f, 0.f,
              "patrol_ship.obj", "ship_mat_guard", true,
              1800.f, false);

    // ── Misc POIs ────────────────────────────────────────────────────────────
    addEntity("DerelictShip_Echo7",
              {"Derelict","POI","Salvage"},
              22.f, -3.f, 0.f,
              "derelict.obj", "mat_derelict", true,
              3000.f, false);

    addEntity("NavigationBeacon_Sol1",
              {"Beacon","Navigation"},
              0.f, -8.f, 0.f);

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

