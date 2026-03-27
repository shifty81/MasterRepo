#pragma once
/**
 * @file TrailRenderer.h
 * @brief Ribbon of quads following a moving point: fade, UV scroll, max segments.
 *
 * Features:
 *   - TrailDesc: maxSegments, segmentLifetime(sec), width, fadeOverLifetime
 *   - AddPoint(pos[3], time): append new trail point
 *   - Tick(dt): advance age of each segment, remove expired ones
 *   - GetMesh: returns quad vertices for rendering (float pos[3], float uv[2], float alpha)
 *   - UV scroll: uOffset scrolled per second along ribbon length
 *   - Width taper: linear narrow toward tail
 *   - Colour gradient over lifetime (head colour → tail colour)
 *   - Multiple trail instances
 *   - Clear(): remove all segments
 *   - SetMinDistance(d): skip AddPoint if delta < d (performance gate)
 *
 * Typical usage:
 * @code
 *   TrailRenderer tr;
 *   tr.Init();
 *   TrailDesc d; d.maxSegments=50; d.segmentLifetime=2.f; d.width=0.15f;
 *   uint32_t id = tr.Create(d);
 *   // each frame:
 *   tr.AddPoint(id, entityPos, currentTime);
 *   tr.Tick(dt);
 *   for (auto& v : tr.GetVertices(id)) render(v);
 * @endcode
 */

#include <cstdint>
#include <vector>

namespace Engine {

struct TrailVertex {
    float pos  [3]{};
    float uv   [2]{};
    float colour[4]{1,1,1,1};
};

struct TrailDesc {
    uint32_t maxSegments    {64};
    float    segmentLifetime{2.f};
    float    width          {0.1f};
    bool     fadeOverLifetime{true};
    bool     taperWidth     {true};
    float    uvScrollSpeed  {0.f};
    float    headColour[4]  {1,1,1,1};
    float    tailColour[4]  {1,1,1,0};
    float    minDistance    {0.01f};
};

class TrailRenderer {
public:
    TrailRenderer();
    ~TrailRenderer();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    uint32_t Create (const TrailDesc& desc);
    void     Destroy(uint32_t id);
    bool     Has    (uint32_t id) const;

    void AddPoint  (uint32_t id, const float pos[3], float time);
    void Clear     (uint32_t id);

    const std::vector<TrailVertex>& GetVertices(uint32_t id) const;
    uint32_t                        VertexCount (uint32_t id) const;
    uint32_t                        SegmentCount(uint32_t id) const;

    // Runtime tweaks
    void SetWidth        (uint32_t id, float w);
    void SetLifetime     (uint32_t id, float sec);
    void SetHeadColour   (uint32_t id, const float rgba[4]);
    void SetTailColour   (uint32_t id, const float rgba[4]);
    void SetUVScrollSpeed(uint32_t id, float speed);

    std::vector<uint32_t> GetAll() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
