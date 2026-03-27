#pragma once
/**
 * @file FactionSystem.h
 * @brief Faction registry, relation matrix (hostile/neutral/ally), reputation, war-declaration.
 *
 * Features:
 *   - Faction definitions: id, name, colour
 *   - Relation matrix: pair → Standing (Hostile, Unfriendly, Neutral, Friendly, Allied)
 *   - Reputation delta: AdjustReputation(factionA, factionB, delta) → clamps [-1000,1000]
 *   - Thresholds for standing transitions: configurable
 *   - GetStanding(a, b) → Standing enum
 *   - IsHostile / IsFriendly / IsAllied helpers
 *   - War declaration: DeclareWar(a, b) → sets both sides hostile, fires callback
 *   - Peace declaration: DeclarePeace(a, b) → resets to Neutral
 *   - Group membership: entity → faction list
 *   - GetFactionOf(entityId) → faction id
 *   - On-standing-changed callback
 *
 * Typical usage:
 * @code
 *   FactionSystem fs;
 *   fs.Init();
 *   fs.RegisterFaction({"humans","Humans"});
 *   fs.RegisterFaction({"undead","Undead"});
 *   fs.SetStanding("humans","undead", Standing::Hostile);
 *   fs.AssignFaction(entityId, "humans");
 *   if (fs.IsHostile("humans","undead")) Attack();
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class Standing : int8_t { Hostile=-2, Unfriendly=-1, Neutral=0, Friendly=1, Allied=2 };

struct FactionDef {
    std::string id;
    std::string name;
    float       colour[3]{1,1,1};
};

class FactionSystem {
public:
    FactionSystem();
    ~FactionSystem();

    void Init    ();
    void Shutdown();

    // Faction management
    void RegisterFaction  (const FactionDef& def);
    void UnregisterFaction(const std::string& id);
    bool HasFaction       (const std::string& id) const;
    const FactionDef* GetFaction(const std::string& id) const;
    std::vector<FactionDef> GetAll() const;

    // Relations
    void     SetStanding     (const std::string& a, const std::string& b, Standing s);
    Standing GetStanding     (const std::string& a, const std::string& b) const;
    void     AdjustReputation(const std::string& a, const std::string& b, float delta);
    float    GetReputation   (const std::string& a, const std::string& b) const;

    bool IsHostile   (const std::string& a, const std::string& b) const;
    bool IsUnfriendly(const std::string& a, const std::string& b) const;
    bool IsFriendly  (const std::string& a, const std::string& b) const;
    bool IsAllied    (const std::string& a, const std::string& b) const;

    void DeclareWar  (const std::string& a, const std::string& b);
    void DeclarePeace(const std::string& a, const std::string& b);

    // Entity membership
    void        AssignFaction  (uint32_t entityId, const std::string& factionId);
    void        RemoveFaction  (uint32_t entityId);
    std::string GetFactionOf   (uint32_t entityId) const;
    bool        EntitiesHostile(uint32_t e1, uint32_t e2) const;

    // Standing thresholds
    void SetThresholds(float unfriendly, float friendly, float allied);

    // Callback
    void SetOnStandingChanged(std::function<void(const std::string& a, const std::string& b,
                                                  Standing oldS, Standing newS)> cb);
private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
