#include "Game/World/GameWorld.h"
#include "Core/Logging/Log.h"

namespace NF::Game {

bool GameWorld::Initialize()
{
    m_Level.Load("game.level");
    m_Ready = true;
    Logger::Log(LogLevel::Info, "GameWorld", "Initialized");
    return true;
}

void GameWorld::Tick(float dt)
{
    if (!m_Ready) return;
    m_Level.Update(dt);
}

void GameWorld::Shutdown()
{
    if (!m_Ready) return;
    m_Level.Unload();
    m_Ready = false;
    Logger::Log(LogLevel::Info, "GameWorld", "Shutdown");
}

} // namespace NF::Game
