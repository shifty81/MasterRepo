#pragma once
/**
 * @file SplineSystem.h
 * @brief Catmull-Rom/Bezier spline: add points, sample position/tangent/normal, arc-length.
 *
 * Features:
 *   - SplineType: CatmullRom, Bezier, Linear
 *   - CreateSpline(id) / DestroySpline(id)
 *   - AddPoint(splineId, pos) / InsertPoint(splineId, index, pos) / RemovePoint(splineId, index)
 *   - GetPointCount(splineId) → uint32_t
 *   - SetClosed(splineId, on) / IsClosed(splineId) → bool
 *   - SamplePosition(splineId, t) → Vec3: t ∈ [0,1]
 *   - SampleTangent(splineId, t) → Vec3 (normalised)
 *   - SampleNormal(splineId, t) → Vec3 (Frenet normal)
 *   - GetLength(splineId, segments) → float: arc-length approximation
 *   - GetTAtLength(splineId, length, segments) → float: inverse arc-length
 *   - Resample(splineId, count, outPoints[]): uniform arc-length resample
 *   - SetType(splineId, type) / GetType(splineId) → SplineType
 *   - SetTension(splineId, tension): Catmull-Rom tension [0,1]
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

struct SplineVec3 { float x, y, z; };

enum class SplineType : uint8_t { Linear, CatmullRom, Bezier };

class SplineSystem {
public:
    SplineSystem();
    ~SplineSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Spline management
    void CreateSpline (uint32_t id);
    void DestroySpline(uint32_t id);

    // Control points
    void     AddPoint   (uint32_t splineId, SplineVec3 pos);
    void     InsertPoint(uint32_t splineId, uint32_t index, SplineVec3 pos);
    void     RemovePoint(uint32_t splineId, uint32_t index);
    uint32_t GetPointCount(uint32_t splineId) const;

    // Config
    void       SetClosed (uint32_t splineId, bool on);
    bool       IsClosed  (uint32_t splineId) const;
    void       SetType   (uint32_t splineId, SplineType type);
    SplineType GetType   (uint32_t splineId) const;
    void       SetTension(uint32_t splineId, float tension);

    // Sampling (t ∈ [0,1])
    SplineVec3 SamplePosition(uint32_t splineId, float t) const;
    SplineVec3 SampleTangent (uint32_t splineId, float t) const;
    SplineVec3 SampleNormal  (uint32_t splineId, float t) const;

    // Arc-length
    float    GetLength       (uint32_t splineId, uint32_t segments = 128) const;
    float    GetTAtLength    (uint32_t splineId, float length, uint32_t segments = 128) const;
    uint32_t Resample        (uint32_t splineId, uint32_t count,
                              std::vector<SplineVec3>& outPoints) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
