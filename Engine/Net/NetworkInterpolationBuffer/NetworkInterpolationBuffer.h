#pragma once
/**
 * @file NetworkInterpolationBuffer.h
 * @brief Client-side interpolation buffer for smooth networked entity rendering.
 *
 * Adapted from Nova-Forge-Expeditions (atlas::net → Engine namespace).
 *
 * Stores recent server snapshots per entity and computes interpolated
 * positions for rendering between server ticks using linear LERP and
 * velocity-based dead-reckoning:
 *
 *   EntitySnapshot: entityId, tick, posX/Y/Z, velX/Y/Z, rotYaw.
 *   InterpolatedState: entityId, posX/Y/Z, rotYaw, valid flag.
 *
 *   NetworkInterpolationBuffer:
 *     - PushSnapshot(snap): record an authoritative server snapshot.
 *     - Interpolate(entityId, renderTick): smooth LERP between snapshots;
 *       dead-reckons beyond the latest snapshot up to maxExtrapolationTicks.
 *     - RemoveEntity(id): clear buffered data for a destroyed entity.
 *     - Clear(): reset all buffers.
 *     - TrackedEntityCount() / SnapshotCount(id).
 *     - BufferDepth (default 8 snapshots per entity).
 *     - MaxExtrapolationTicks (default 3.0 ticks before returning invalid).
 */

#include <cstdint>
#include <vector>
#include <cmath>

namespace Engine {

// ── Entity snapshot ───────────────────────────────────────────────────────
struct EntitySnapshot {
    uint32_t entityId{0};
    uint32_t tick{0};
    float    posX{0}, posY{0}, posZ{0};
    float    velX{0}, velY{0}, velZ{0};
    float    rotYaw{0};
};

// ── Interpolated visual state ─────────────────────────────────────────────
struct InterpolatedState {
    uint32_t entityId{0};
    float    posX{0}, posY{0}, posZ{0};
    float    rotYaw{0};
    bool     valid{false};
};

// ── Buffer ────────────────────────────────────────────────────────────────
class NetworkInterpolationBuffer {
public:
    explicit NetworkInterpolationBuffer(size_t bufferDepth = 8,
                                        float  maxExtrapolationTicks = 3.0f);
    ~NetworkInterpolationBuffer() = default;

    // ── ingestion ─────────────────────────────────────────────
    void PushSnapshot(const EntitySnapshot& snap);

    // ── rendering query ───────────────────────────────────────
    /// Interpolate the visual state of an entity at renderTick (fractional).
    InterpolatedState Interpolate(uint32_t entityId, float renderTick) const;

    // ── management ────────────────────────────────────────────
    void RemoveEntity(uint32_t entityId);
    void Clear();

    // ── info ──────────────────────────────────────────────────
    size_t TrackedEntityCount()              const;
    size_t SnapshotCount(uint32_t entityId)  const;
    size_t BufferDepth()                     const { return m_bufferDepth; }
    float  MaxExtrapolationTicks()           const { return m_maxExtrapolationTicks; }

private:
    struct EntityBuffer {
        uint32_t                    entityId{0};
        std::vector<EntitySnapshot> snapshots; // oldest first
    };

    EntityBuffer*       findEntity(uint32_t id);
    const EntityBuffer* findEntity(uint32_t id) const;

    static InterpolatedState lerp(const EntitySnapshot& a,
                                  const EntitySnapshot& b, float t);
    static InterpolatedState extrapolate(const EntitySnapshot& snap,
                                         float ticksBeyond);

    std::vector<EntityBuffer> m_entities;
    size_t m_bufferDepth;
    float  m_maxExtrapolationTicks;
};

} // namespace Engine
