#include "Game/App/Orchestrator.h"
#include "Core/Logging/Log.h"

namespace NF::Game {

bool Orchestrator::Init(RenderDevice* renderDevice)
{
    m_RenderDevice = renderDevice;

    m_Level.Load("game.level");

    m_Initialized = true;
    Logger::Log(LogLevel::Info, "Game", "Orchestrator::Init complete");
    return true;
}

void Orchestrator::Tick(float dt)
{
    if (!m_Initialized) return;

    m_Level.Update(dt);

    if (m_RenderDevice)
    {
        m_RenderDevice->BeginFrame();
        m_RenderDevice->Clear(0.05f, 0.05f, 0.08f, 1.0f);
        m_RenderDevice->EndFrame();
    }
}

void Orchestrator::Shutdown()
{
    if (!m_Initialized) return;

    m_Level.Unload();
    m_RenderDevice = nullptr;
    m_Initialized  = false;
    Logger::Log(LogLevel::Info, "Game", "Orchestrator::Shutdown");
}

} // namespace NF::Game
