#pragma once
#include <cstdint>
#include <vector>
#include <cmath>

namespace atlas::net {

/**
 * A position/velocity snapshot for a single networked entity at a point in time.
 */
struct EntitySnapshot {
    uint32_t entityId = 0;
    uint32_t tick     = 0;
    float    posX     = 0.0f;
    float    posY     = 0.0f;
    float    posZ     = 0.0f;
    float    velX     = 0.0f;
    float    velY     = 0.0f;
    float    velZ     = 0.0f;
    float    rotYaw   = 0.0f;
};

/**
 * Interpolated result for rendering — the smooth visual state of an entity.
 */
struct InterpolatedState {
    uint32_t entityId = 0;
    float    posX     = 0.0f;
    float    posY     = 0.0f;
    float    posZ     = 0.0f;
    float    rotYaw   = 0.0f;
    bool     valid    = false;
};

/**
 * @brief Client-side interpolation buffer for smooth entity rendering.
 *
 * Stores recent server snapshots per entity and computes interpolated
 * positions for rendering between server ticks.  Uses linear LERP for
 * position and optional velocity-based dead-reckoning when snapshots
 * are missing.
 *
 * Workflow:
 *   1. Server sends authoritative snapshots each tick.
 *   2. Client calls PushSnapshot() on receipt.
 *   3. Each render frame, client calls Interpolate() with the current
 *      render time (in ticks, fractional) to get a smooth position.
 *
 * Configuration:
 *   - bufferDepth: number of snapshots to keep per entity (default 8)
 *   - maxExtrapolationTicks: how far to dead-reckon beyond the latest
 *     snapshot before returning invalid (default 3)
 */
class NetworkInterpolationBuffer {
public:
    explicit NetworkInterpolationBuffer(size_t bufferDepth = 8,
                                         float maxExtrapolationTicks = 3.0f);

    /** Record a new authoritative snapshot for an entity. */
    void PushSnapshot(const EntitySnapshot& snap);

    /**
     * Compute the interpolated visual state of an entity at the given
     * render tick (may be fractional, e.g. 102.4).
     *
     * Returns InterpolatedState with valid=true if interpolation succeeded,
     * or valid=false if the entity has no data / has timed out.
     */
    InterpolatedState Interpolate(uint32_t entityId, float renderTick) const;

    /** Remove all snapshots for a specific entity. */
    void RemoveEntity(uint32_t entityId);

    /** Clear all buffered data. */
    void Clear();

    /** Number of entities currently tracked. */
    size_t TrackedEntityCount() const { return m_entities.size(); }

    /** Number of snapshots buffered for an entity. */
    size_t SnapshotCount(uint32_t entityId) const;

    /** Get buffer depth setting. */
    size_t BufferDepth() const { return m_bufferDepth; }

    /** Get max extrapolation ticks setting. */
    float MaxExtrapolationTicks() const { return m_maxExtrapolationTicks; }

private:
    struct EntityBuffer {
        uint32_t entityId = 0;
        std::vector<EntitySnapshot> snapshots; // oldest first
    };

    EntityBuffer*       findEntity(uint32_t id);
    const EntityBuffer* findEntity(uint32_t id) const;

    /** Linear interpolation between two snapshots. */
    static InterpolatedState lerp(const EntitySnapshot& a,
                                  const EntitySnapshot& b,
                                  float t);

    /** Dead-reckoning extrapolation from a single snapshot. */
    static InterpolatedState extrapolate(const EntitySnapshot& snap,
                                         float ticksBeyond);

    std::vector<EntityBuffer> m_entities;
    size_t m_bufferDepth;
    float  m_maxExtrapolationTicks;
};

} // namespace atlas::net
