#pragma once
#include "Core/Math/Vector.h"
#include <cstdint>
#include <vector>

namespace NF {

/// @brief Classification of a light source.
enum class LightType { Directional, Point, Spot };

/// @brief Opaque handle identifying a light in the LightSystem.
using LightId = uint32_t;

/// @brief Data for a single light source in the scene.
struct Light {
    LightType Type{LightType::Point};
    Vector3   Position{};
    Vector3   Direction{0.f, -1.f, 0.f};
    Vector3   Color{1.f, 1.f, 1.f};
    float     Intensity{1.f};
    float     Range{10.f};
};

/// @brief Manages the set of active lights and drives per-frame updates.
class LightSystem {
public:
    /// @brief Register a light with the system and return its unique id.
    /// @param light Light data to add.
    /// @return The id assigned to this light.
    LightId AddLight(Light light);

    /// @brief Remove the light with the given id (no-op if not found).
    void RemoveLight(LightId id);

    /// @brief Return a read-only view of all currently active lights.
    [[nodiscard]] const std::vector<Light>& GetLights() const noexcept;

    /// @brief Advance any animated or dynamic lights by @p dt seconds.
    /// @param dt Delta time in seconds.
    void Update(float dt);

private:
    struct LightEntry {
        LightId Id{0};
        Light   Data{};
    };

    std::vector<LightEntry> m_Lights;
    mutable std::vector<Light> m_FlatCache;
    LightId                    m_NextId{1};
};

} // namespace NF
