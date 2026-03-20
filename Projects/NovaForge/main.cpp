#include "Engine/Core/Engine.h"
#include <iostream>

int main() {
    Engine::Core::EngineConfig cfg;
    cfg.mode = Engine::Core::EngineMode::Client;

    Engine::Core::Engine engine(cfg);
    engine.InitCore();
    engine.InitRender();
    engine.InitECS();

    std::cout << "[NovaForge] Game initialized\n";
    engine.Run();
    std::cout << "[NovaForge] Exited\n";
    return 0;
}
