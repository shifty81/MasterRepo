#include "engine/Game.h"
#include "engine/Logger.h"
#include <iostream>
#include <memory>

int main(int argc, char* argv[]) {
    // Initialize logging before anything else
    Logger::Instance().Initialize("harvest_quest.log");

    Logger::Instance().Info("==================================");
    Logger::Instance().Info("   Harvest Quest - Alpha Build   ");
    Logger::Instance().Info("==================================");
    Logger::Instance().Info("A game combining Zelda: A Link to the Past");
    Logger::Instance().Info("and Stardew Valley mechanics!");
    Logger::Instance().Info("");
    Logger::Instance().Info("Controls:");
    Logger::Instance().Info("  WASD / Arrow Keys - Move");
    Logger::Instance().Info("  Space - Attack");
    Logger::Instance().Info("  T/R/F/H - Farm (Till/Water/Plant/Harvest)");
    Logger::Instance().Info("  C - Chop tree | G - Fish");
    Logger::Instance().Info("  N - Advance day (sleep) | I - Inventory");
    Logger::Instance().Info("  P - Crafting | E - Talk to NPC");
    Logger::Instance().Info("  F5 - Save | F9 - Load");
    Logger::Instance().Info("  1/2/3 - Generate Farm/Dungeon/Overworld");
    Logger::Instance().Info("  ESC - Quit");
    Logger::Instance().Info("==================================");

    auto game = std::make_unique<Game>();

    if (!game->Initialize("Harvest Quest - Zelda meets Stardew Valley", 800, 600)) {
        Logger::Instance().Error("Failed to initialize game!");
        Logger::Instance().Shutdown();
        return 1;
    }

    game->Run();
    game->Shutdown();

    Logger::Instance().Info("Thanks for playing!");
    Logger::Instance().Shutdown();
    return 0;
}
