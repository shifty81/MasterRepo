#include "Game/World/WorldDebugOverlay.h"
#include "Core/Logging/Log.h"
#include <sstream>

namespace NF::Game {

void WorldDebugOverlay::Update(const DevWorldConfig& config,
                               const World* world,
                               EntityId playerEntityId)
{
    m_Lines.clear();

    if (!m_Enabled) return;

    // World identity
    m_Lines.push_back("=== World Debug Overlay ===");
    m_Lines.push_back("World ID   : " + config.WorldId());
    m_Lines.push_back("Display    : " + config.DisplayName());
    m_Lines.push_back("Seed       : " + std::to_string(config.Seed()));

    // Terrain
    {
        const auto& ts = config.TerrainSize();
        std::ostringstream oss;
        oss << "Terrain    : " << ts.X << " x " << ts.Y << " x " << ts.Z;
        m_Lines.push_back(oss.str());
    }
    m_Lines.push_back("Gravity    : " + std::to_string(config.Gravity()));

    // Spawn point
    {
        const auto& sp = config.GetSpawnPoint();
        std::ostringstream oss;
        oss << "Spawn      : (" << sp.Position.X << ", "
            << sp.Position.Y << ", " << sp.Position.Z << ")";
        m_Lines.push_back(oss.str());
    }

    // Camera config
    {
        const auto& cam = config.GetCameraConfig();
        m_Lines.push_back("FOV        : " + std::to_string(cam.FOV));
        m_Lines.push_back("Near/Far   : " + std::to_string(cam.NearClip)
                          + " / " + std::to_string(cam.FarClip));
    }

    // ECS stats
    if (world) {
        // We don't have a public entity count accessor; approximate via player check.
        m_Lines.push_back("Player Ent : " + std::to_string(playerEntityId));
    } else {
        m_Lines.push_back("World      : (no world loaded)");
    }

    m_Lines.push_back("===========================");
}

void WorldDebugOverlay::LogToConsole() const
{
    for (const auto& line : m_Lines)
        Logger::Log(LogLevel::Debug, "DebugOverlay", line);
}

} // namespace NF::Game
