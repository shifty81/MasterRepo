#pragma once
/**
 * @file CityGenerator.h
 * @brief Procedural city generation: road network, blocks, lots, zoning.
 *
 * CityGenerator produces:
 *   - RoadSegment: directed edge between two Intersection nodes
 *   - Intersection: at-grade crossing point; classified by valence
 *   - CityBlock: polygon region bounded by road segments
 *   - Lot: subdivision of a block with assigned land use (Residential, Commercial,
 *     Industrial, Park, Civic, Empty)
 *
 * Algorithm:
 *   1. Seed major arteries along a grid (or radial) skeleton.
 *   2. Subdivide with secondary streets via L-System-style iteration.
 *   3. Extract block polygons from road graph planar faces.
 *   4. Subdivide blocks into lots with configurable lot-width range.
 *   5. Assign land use based on distance-from-centre and random weights.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace PCG {

// ── Coordinates ────────────────────────────────────────────────────────────
struct CityVec2 { float x{0.0f}, y{0.0f}; };

// ── Road types ─────────────────────────────────────────────────────────────
enum class RoadType : uint8_t { Highway, Arterial, Secondary, Residential, Alley };

struct Intersection {
    uint32_t id{0};
    CityVec2 pos;
    uint32_t valence{0};  ///< Number of connected road segments
};

struct RoadSegment {
    uint32_t  id{0};
    uint32_t  fromId{0};  ///< Intersection id
    uint32_t  toId{0};
    RoadType  type{RoadType::Secondary};
    float     width{8.0f};  ///< metres
    float     length{0.0f};
};

// ── Land use ───────────────────────────────────────────────────────────────
enum class LandUse : uint8_t {
    Residential, Commercial, Industrial, Park, Civic, Empty
};

struct Lot {
    uint32_t          id{0};
    std::vector<CityVec2> corners;  ///< Ordered polygon vertices
    LandUse           use{LandUse::Empty};
    float             area{0.0f};
    uint32_t          blockId{0};
};

struct CityBlock {
    uint32_t          id{0};
    std::vector<CityVec2> outline;    ///< Block boundary polygon
    std::vector<uint32_t> lotIds;
    float             area{0.0f};
};

// ── Generator config ───────────────────────────────────────────────────────
struct CityConfig {
    float    worldSize{1000.0f};    ///< Total city extent (square, metres)
    uint32_t gridColumns{10};
    uint32_t gridRows{10};
    float    blockWidth{80.0f};
    float    blockDepth{80.0f};
    float    arterialWidth{16.0f};
    float    secondaryWidth{10.0f};
    float    residentialWidth{7.0f};
    float    minLotWidth{8.0f};
    float    maxLotWidth{20.0f};
    float    commercialCoreBias{0.5f};  ///< 0=uniform 1=dense commercial centre
    uint64_t seed{42};
};

// ── Result ─────────────────────────────────────────────────────────────────
struct CityLayout {
    std::vector<Intersection> intersections;
    std::vector<RoadSegment>  roads;
    std::vector<CityBlock>    blocks;
    std::vector<Lot>          lots;

    size_t IntersectionCount() const { return intersections.size(); }
    size_t RoadCount()         const { return roads.size(); }
    size_t BlockCount()        const { return blocks.size(); }
    size_t LotCount()          const { return lots.size(); }
};

// ── Generator ──────────────────────────────────────────────────────────────
class CityGenerator {
public:
    CityGenerator();
    ~CityGenerator();

    void SetConfig(const CityConfig& cfg);
    const CityConfig& Config() const;

    /// Generate a complete city layout synchronously.
    CityLayout Generate();

    /// Step-by-step generation; returns true when finished.
    bool GenerateStep();
    bool IsFinished() const;
    const CityLayout& CurrentLayout() const;

    /// Progress callback called after each major step.
    using ProgressCb = std::function<void(float fraction, const std::string& step)>;
    void OnProgress(ProgressCb cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace PCG
