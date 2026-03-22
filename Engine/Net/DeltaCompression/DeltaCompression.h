#pragma once
/**
 * @file DeltaCompression.h
 * @brief Server-side delta compression for networked entity state snapshots.
 *
 * Adapted from Nova-Forge-Expeditions (atlas::net → Engine namespace).
 *
 * Converts full EntitySnapshot data into compact delta-encoded frames.
 * The encoder maintains a per-entity baseline; each snapshot is emitted as:
 *   - Keyframe (FrameType::Keyframe): full quantised state, resets baseline.
 *   - Delta    (FrameType::Delta):    difference from the current baseline.
 *
 * Keyframes are forced every keyframeInterval ticks, on first encode,
 * or when explicitly requested via ForceKeyframe() / ForceAllKeyframes().
 *
 * Quantisation precision:
 *   Position / Velocity: 0.01 unit  (×100 fixed-point)
 *   Rotation (yaw):      0.1 degree (×10  fixed-point)
 *
 * Workflow:
 *   Server tick  → Encode(snapshots) → CompressedFrame → transmit
 *   Client recv  → Decode(frame)     → vector<EntitySnapshot>
 */

#include "Engine/Net/NetworkInterpolationBuffer/NetworkInterpolationBuffer.h"
#include <cstdint>
#include <vector>
#include <unordered_map>

namespace Engine {

// ── Frame type ────────────────────────────────────────────────────────────
enum class FrameType : uint8_t { Keyframe = 0, Delta = 1 };

// ── Field delta (fixed-point) ─────────────────────────────────────────────
struct FieldDelta {
    int32_t dPosX{0}, dPosY{0}, dPosZ{0};
    int32_t dVelX{0}, dVelY{0}, dVelZ{0};
    int32_t dRotYaw{0};
};

// ── Per-entity snapshot in a frame ────────────────────────────────────────
struct CompressedSnapshot {
    uint32_t   entityId{0};
    uint32_t   tick{0};
    FrameType  frameType{FrameType::Keyframe};
    FieldDelta delta;
};

// ── Full compressed frame ─────────────────────────────────────────────────
struct CompressedFrame {
    uint32_t                       tick{0};
    std::vector<CompressedSnapshot> entries;
};

// ── DeltaCompression ──────────────────────────────────────────────────────
class DeltaCompression {
public:
    static constexpr float kPositionScale = 100.0f;
    static constexpr float kRotationScale = 10.0f;

    explicit DeltaCompression(uint32_t keyframeInterval = 30);

    // ── encode / decode ───────────────────────────────────────
    CompressedFrame              Encode(const std::vector<EntitySnapshot>& snapshots);
    std::vector<EntitySnapshot>  Decode(const CompressedFrame& frame);

    // ── control ───────────────────────────────────────────────
    void ForceKeyframe(uint32_t entityId);
    void ForceAllKeyframes();
    void RemoveEntity(uint32_t entityId);
    void Clear();

    // ── info ──────────────────────────────────────────────────
    size_t   BaselineCount()    const { return m_baselines.size(); }
    uint32_t KeyframeInterval() const { return m_keyframeInterval; }

    // ── quantisation helpers (public for tests) ───────────────
    static int32_t QuantizePosition(float v);
    static float   DequantizePosition(int32_t v);
    static int32_t QuantizeRotation(float deg);
    static float   DequantizeRotation(int32_t v);

private:
    struct Baseline {
        EntitySnapshot snapshot;
        uint32_t       lastKeyframeTick{0};
        bool           forceKeyframe{false};
    };

    bool needsKeyframe(uint32_t entityId, uint32_t currentTick) const;

    std::unordered_map<uint32_t, Baseline> m_baselines;
    uint32_t m_keyframeInterval;
    bool     m_forceAll{false};
};

} // namespace Engine
