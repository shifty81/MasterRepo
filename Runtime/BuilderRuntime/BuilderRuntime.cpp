#include "Runtime/BuilderRuntime/BuilderRuntime.h"
#include <cstring>

namespace Runtime {

void BuilderRuntime::Init(Builder::Assembly* assembly, Builder::PartLibrary* library) {
    m_assembly = assembly;
    m_partLib  = library;
    m_phase    = BuildPhase::Idle;
    m_building = false;
    m_elapsed  = 0.f;
}

void BuilderRuntime::Tick(float dt) {
    m_elapsed += dt;
}

void BuilderRuntime::BeginBuild() {
    m_building = true;
    m_phase    = BuildPhase::Idle;
}

void BuilderRuntime::EndBuild() {
    m_building = false;
    m_phase    = BuildPhase::Idle;
}

bool BuilderRuntime::IsBuilding() const { return m_building; }

uint32_t BuilderRuntime::PlacePart(uint32_t defId, const float transform[16]) {
    if (!m_building || !m_assembly || !m_partLib) return 0;
    if (m_assembly->PartCount() >= m_maxParts) return 0;
    if (!m_partLib->Get(defId)) return 0;
    m_phase = BuildPhase::PlacingPart;
    uint32_t instId = m_assembly->AddPart(defId, transform);
    m_phase = BuildPhase::Idle;
    LogEvent(RuntimeBuildEvent::Type::PartPlaced, instId);
    return instId;
}

bool BuilderRuntime::RemovePart(uint32_t instanceId) {
    if (!m_building || !m_assembly) return false;
    if (!m_assembly->GetPart(instanceId)) return false;
    m_phase = BuildPhase::Dismantling;
    m_assembly->RemovePart(instanceId);
    m_phase = BuildPhase::Idle;
    LogEvent(RuntimeBuildEvent::Type::PartRemoved, instanceId);
    return true;
}

bool BuilderRuntime::WeldParts(uint32_t instA, uint32_t snapA, uint32_t instB, uint32_t snapB) {
    if (!m_building || !m_assembly) return false;
    m_phase = BuildPhase::ConfirmingWeld;
    bool ok = m_assembly->Link(instA, snapA, instB, snapB, true);
    m_phase = BuildPhase::Idle;
    if (ok) LogEvent(RuntimeBuildEvent::Type::WeldApplied, instA);
    return ok;
}

bool BuilderRuntime::UnweldParts(uint32_t instA, uint32_t snapA) {
    if (!m_building || !m_assembly) return false;
    m_assembly->Unlink(instA, snapA);
    LogEvent(RuntimeBuildEvent::Type::WeldRemoved, instA);
    return true;
}

BuildPhase BuilderRuntime::GetPhase() const { return m_phase; }

const std::vector<RuntimeBuildEvent>& BuilderRuntime::GetEventLog() const { return m_eventLog; }
void BuilderRuntime::ClearEventLog() { m_eventLog.clear(); }

void BuilderRuntime::SetMaxParts(uint32_t n) { m_maxParts = n; }
uint32_t BuilderRuntime::GetMaxParts()  const { return m_maxParts; }
uint32_t BuilderRuntime::GetPartCount() const { return m_assembly ? (uint32_t)m_assembly->PartCount() : 0; }

void BuilderRuntime::LogEvent(RuntimeBuildEvent::Type type, uint32_t instanceId) {
    RuntimeBuildEvent ev;
    ev.type       = type;
    ev.instanceId = instanceId;
    ev.timestamp  = m_elapsed;
    m_eventLog.push_back(ev);
}

} // namespace Runtime
