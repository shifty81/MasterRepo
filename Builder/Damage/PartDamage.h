#pragma once
#include "Builder/Assembly/Assembly.h"
#include "Builder/Parts/PartDef.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace Builder {

struct DamageEvent {
    uint32_t    targetInstanceId = 0;
    float       amount           = 0.0f;
    std::string type;
    uint32_t    sourceId         = 0;
};

struct PartHealthState {
    uint32_t    instanceId = 0;
    float       currentHP  = 0.0f;
    float       maxHP      = 0.0f;
    bool        destroyed  = false;
    std::vector<std::string> effects;
};

class BuilderDamageSystem {
public:
    void InitFromAssembly(const Assembly& assembly, const PartLibrary& library);
    PartHealthState  ApplyDamage(const DamageEvent& evt);
    void             RepairPart(uint32_t instanceId, float amount);
    bool             IsDestroyed(uint32_t instanceId) const;
    PartHealthState* GetState(uint32_t instanceId);
    const PartHealthState* GetState(uint32_t instanceId) const;
    std::vector<uint32_t> GetDestroyedParts() const;
    void Reset();

private:
    std::unordered_map<uint32_t, PartHealthState> m_states;
};

} // namespace Builder
