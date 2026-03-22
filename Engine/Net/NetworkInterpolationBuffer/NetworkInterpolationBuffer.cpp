#include "Engine/Net/NetworkInterpolationBuffer/NetworkInterpolationBuffer.h"
#include <algorithm>

namespace Engine {

NetworkInterpolationBuffer::NetworkInterpolationBuffer(size_t bufferDepth,
                                                        float maxExtrapolationTicks)
    : m_bufferDepth(bufferDepth > 0 ? bufferDepth : 1)
    , m_maxExtrapolationTicks(maxExtrapolationTicks)
{}

void NetworkInterpolationBuffer::PushSnapshot(const EntitySnapshot& snap) {
    EntityBuffer* buf = findEntity(snap.entityId);
    if (!buf) {
        m_entities.push_back({snap.entityId, {}});
        buf = &m_entities.back();
    }
    // Insert in tick order
    auto& snaps = buf->snapshots;
    auto pos = std::lower_bound(snaps.begin(), snaps.end(), snap,
        [](const EntitySnapshot& a, const EntitySnapshot& b){
            return a.tick < b.tick;
        });
    snaps.insert(pos, snap);
    // Trim to buffer depth
    while (snaps.size() > m_bufferDepth) snaps.erase(snaps.begin());
}

InterpolatedState NetworkInterpolationBuffer::Interpolate(uint32_t entityId,
                                                            float renderTick) const {
    const EntityBuffer* buf = findEntity(entityId);
    if (!buf || buf->snapshots.empty()) {
        return {entityId, 0, 0, 0, 0, false};
    }
    const auto& snaps = buf->snapshots;

    // Find the two snapshots straddling renderTick
    for (size_t i = 0; i + 1 < snaps.size(); ++i) {
        if (snaps[i].tick <= renderTick && snaps[i+1].tick >= renderTick) {
            float range = static_cast<float>(snaps[i+1].tick - snaps[i].tick);
            float t = (range > 0) ? (renderTick - snaps[i].tick) / range : 0.0f;
            return lerp(snaps[i], snaps[i+1], t);
        }
    }

    // Beyond the latest snapshot — dead reckon
    const auto& latest = snaps.back();
    float beyond = renderTick - static_cast<float>(latest.tick);
    if (beyond > m_maxExtrapolationTicks) {
        return {entityId, 0, 0, 0, 0, false};
    }
    return extrapolate(latest, beyond);
}

void NetworkInterpolationBuffer::RemoveEntity(uint32_t entityId) {
    m_entities.erase(
        std::remove_if(m_entities.begin(), m_entities.end(),
            [entityId](const EntityBuffer& b){ return b.entityId == entityId; }),
        m_entities.end());
}

void NetworkInterpolationBuffer::Clear() { m_entities.clear(); }

size_t NetworkInterpolationBuffer::TrackedEntityCount() const {
    return m_entities.size();
}

size_t NetworkInterpolationBuffer::SnapshotCount(uint32_t entityId) const {
    const EntityBuffer* buf = findEntity(entityId);
    return buf ? buf->snapshots.size() : 0;
}

// ── private helpers ───────────────────────────────────────────────────────

NetworkInterpolationBuffer::EntityBuffer*
NetworkInterpolationBuffer::findEntity(uint32_t id) {
    for (auto& e : m_entities) if (e.entityId == id) return &e;
    return nullptr;
}

const NetworkInterpolationBuffer::EntityBuffer*
NetworkInterpolationBuffer::findEntity(uint32_t id) const {
    for (const auto& e : m_entities) if (e.entityId == id) return &e;
    return nullptr;
}

InterpolatedState NetworkInterpolationBuffer::lerp(
    const EntitySnapshot& a, const EntitySnapshot& b, float t)
{
    auto lf = [](float x, float y, float tt){ return x + tt * (y - x); };
    // Shortest-path yaw lerp
    float dy = b.rotYaw - a.rotYaw;
    while (dy >  180.0f) dy -= 360.0f;
    while (dy < -180.0f) dy += 360.0f;
    return {a.entityId,
            lf(a.posX, b.posX, t), lf(a.posY, b.posY, t), lf(a.posZ, b.posZ, t),
            a.rotYaw + dy * t, true};
}

InterpolatedState NetworkInterpolationBuffer::extrapolate(
    const EntitySnapshot& snap, float ticksBeyond)
{
    return {snap.entityId,
            snap.posX + snap.velX * ticksBeyond,
            snap.posY + snap.velY * ticksBeyond,
            snap.posZ + snap.velZ * ticksBeyond,
            snap.rotYaw, true};
}

} // namespace Engine
