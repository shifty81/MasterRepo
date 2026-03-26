#pragma once
/**
 * @file SplineSystem.h
 * @brief Catmull-Rom and Bezier spline system for paths, rails, and animation curves.
 *
 * Supports:
 *   - Catmull-Rom splines (automatically computed tangents)
 *   - Cubic Bezier curves (explicit control handles)
 *   - Closed / open loops
 *   - Uniform arc-length parameterization (constant speed travel)
 *   - Spline evaluation: position, tangent, normal, binormal at parameter t
 *   - Frenet frame computation
 *   - JSON serialization
 *
 * Typical usage:
 * @code
 *   SplineSystem ss;
 *   ss.Init();
 *   uint32_t id = ss.CreateSpline("rail_01", SplineType::CatmullRom);
 *   ss.AddPoint(id, {0,0,0});
 *   ss.AddPoint(id, {10,0,5});
 *   ss.AddPoint(id, {20,0,0});
 *   float pos[3], tan[3];
 *   ss.Evaluate(id, 0.5f, pos, tan);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class SplineType : uint8_t { CatmullRom=0, CubicBezier=1, Linear=2 };

struct SplinePoint {
    float pos[3]{};
    float handleIn[3]{};   ///< Bezier only
    float handleOut[3]{};  ///< Bezier only
    float roll{0.f};       ///< bank angle degrees
};

struct SplineInfo {
    uint32_t   id{0};
    std::string name;
    SplineType  type{SplineType::CatmullRom};
    bool        closed{false};
    float       totalLength{0.f};
    uint32_t    pointCount{0};
};

class SplineSystem {
public:
    SplineSystem();
    ~SplineSystem();

    void Init();
    void Shutdown();

    uint32_t CreateSpline(const std::string& name,
                           SplineType type = SplineType::CatmullRom,
                           bool closed = false);
    void     DestroySpline(uint32_t id);
    bool     HasSpline(uint32_t id) const;
    SplineInfo GetInfo(uint32_t id) const;
    std::vector<SplineInfo> AllSplines() const;

    // Point editing
    void     AddPoint(uint32_t id, const float pos[3]);
    void     InsertPoint(uint32_t id, uint32_t index, const float pos[3]);
    void     SetPoint(uint32_t id, uint32_t index, const SplinePoint& pt);
    void     RemovePoint(uint32_t id, uint32_t index);
    SplinePoint GetPoint(uint32_t id, uint32_t index) const;
    uint32_t    PointCount(uint32_t id) const;

    void SetClosed(uint32_t id, bool closed);
    void RebuildCache(uint32_t id);     ///< recompute arc-length table

    // Evaluation (t in [0,1])
    void Evaluate(uint32_t id, float t,
                  float pos[3],
                  float tangent[3] = nullptr,
                  float normal[3]  = nullptr,
                  float binormal[3]= nullptr) const;

    float    TotalLength(uint32_t id) const;
    float    DistanceToT(uint32_t id, float distance) const; ///< arc-length → t

    // Nearest point on spline
    float    NearestT(uint32_t id, const float worldPos[3]) const;

    // Serialization
    bool SaveToFile(const std::string& path) const;
    bool LoadFromFile(const std::string& path);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
