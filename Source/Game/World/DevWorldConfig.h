#pragma once
#include "Core/Math/Vector.h"
#include "Core/Math/Quaternion.h"
#include <cstdint>
#include <string>

namespace NF::Game {

/// @brief Fixed spawn point definition for a world.
struct SpawnPoint {
    Vector3    Position{0.f, 0.f, 0.f};
    Quaternion Rotation{0.f, 0.f, 0.f, 1.f};
    Vector3    LookDirection{0.f, 0.f, 1.f};
};

/// @brief Camera configuration for a world.
struct CameraConfig {
    float FOV            = 75.0f;
    float NearClip       = 0.1f;
    float FarClip        = 1000.0f;
    float MoveSpeed      = 10.0f;
    float LookSensitivity = 0.3f;
};

/// @brief Configuration for the development sandbox world.
///
/// Loaded from Content/Definitions/DevWorld.json.  Provides a fixed seed,
/// spawn point, and camera settings so the same world loads repeatably.
class DevWorldConfig {
public:
    DevWorldConfig() = default;

    /// @brief Load configuration from a JSON file on disk.
    /// @param path Absolute or project-relative path to the JSON file.
    /// @return True on success.
    bool LoadFromFile(const std::string& path);

    /// @brief Return a config populated with compiled-in defaults.
    static DevWorldConfig Defaults();

    // -- Identity -------------------------------------------------------------
    [[nodiscard]] const std::string& WorldId()      const noexcept { return m_WorldId; }
    [[nodiscard]] const std::string& DisplayName()  const noexcept { return m_DisplayName; }
    [[nodiscard]] const std::string& Description()  const noexcept { return m_Description; }

    // -- World parameters -----------------------------------------------------
    [[nodiscard]] uint32_t Seed()       const noexcept { return m_Seed; }
    [[nodiscard]] Vector3  TerrainSize() const noexcept { return m_TerrainSize; }
    [[nodiscard]] float    Gravity()     const noexcept { return m_Gravity; }

    // -- Spawn ----------------------------------------------------------------
    [[nodiscard]] const SpawnPoint& GetSpawnPoint() const noexcept { return m_Spawn; }

    // -- Camera ---------------------------------------------------------------
    [[nodiscard]] const CameraConfig& GetCameraConfig() const noexcept { return m_Camera; }

    /// @brief Returns true after a successful LoadFromFile() or Defaults().
    [[nodiscard]] bool IsValid() const noexcept { return m_Valid; }

private:
    std::string  m_WorldId{"DevWorld"};
    std::string  m_DisplayName{"Development Sandbox"};
    std::string  m_Description;
    uint32_t     m_Seed{42};
    Vector3      m_TerrainSize{256.f, 64.f, 256.f};
    float        m_Gravity{-9.81f};
    SpawnPoint   m_Spawn{{128.f, 32.f, 128.f}, {0.f, 0.f, 0.f, 1.f}, {0.f, 0.f, 1.f}};
    CameraConfig m_Camera;
    bool         m_Valid{false};
};

} // namespace NF::Game
