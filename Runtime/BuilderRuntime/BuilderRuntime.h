#pragma once
#include "Builder/Assembly/Assembly.h"
#include "Builder/Parts/PartDef.h"
#include <cstdint>
#include <vector>

namespace Runtime {

enum class BuildPhase { Idle, PlacingPart, SnappingPart, ConfirmingWeld, Dismantling };

struct RuntimeBuildEvent {
    enum class Type { PartPlaced, PartRemoved, WeldApplied, WeldRemoved };
    Type     type        = Type::PartPlaced;
    uint32_t instanceId  = 0;
    float    timestamp   = 0.f;
};

class BuilderRuntime {
public:
    void Init(Builder::Assembly* assembly, Builder::PartLibrary* library);
    void Tick(float dt);

    void BeginBuild();
    void EndBuild();
    bool IsBuilding() const;

    uint32_t PlacePart(uint32_t defId, const float transform[16]);
    bool     RemovePart(uint32_t instanceId);
    bool     WeldParts(uint32_t instA, uint32_t snapA, uint32_t instB, uint32_t snapB);
    bool     UnweldParts(uint32_t instA, uint32_t snapA);

    BuildPhase GetPhase() const;

    const std::vector<RuntimeBuildEvent>& GetEventLog() const;
    void ClearEventLog();

    void     SetMaxParts(uint32_t n);
    uint32_t GetMaxParts()  const;
    uint32_t GetPartCount() const;

private:
    void LogEvent(RuntimeBuildEvent::Type type, uint32_t instanceId);

    Builder::Assembly*    m_assembly   = nullptr;
    Builder::PartLibrary* m_partLib    = nullptr;
    BuildPhase            m_phase      = BuildPhase::Idle;
    bool                  m_building   = false;
    uint32_t              m_maxParts   = 512;
    float                 m_elapsed    = 0.f;
    std::vector<RuntimeBuildEvent> m_eventLog;
};

} // namespace Runtime
