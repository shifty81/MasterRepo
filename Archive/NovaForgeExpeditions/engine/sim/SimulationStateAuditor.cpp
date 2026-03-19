#include "SimulationStateAuditor.h"
#include <algorithm>

namespace atlas::sim {

void SimulationStateAuditor::RegisterSystem(const std::string& name) {
    for (const auto& s : m_systems) {
        if (s == name) return;  // already registered
    }
    m_systems.push_back(name);
}

void SimulationStateAuditor::DeclareState(const std::string& systemName,
                                           const std::string& blockName,
                                           StateCategory category) {
    m_declarations.push_back({systemName, blockName, category});
}

const std::vector<std::string>& SimulationStateAuditor::RegisteredSystems() const {
    return m_systems;
}

size_t SimulationStateAuditor::DeclaredStateCount(const std::string& systemName) const {
    size_t count = 0;
    for (const auto& d : m_declarations) {
        if (d.systemName == systemName) ++count;
    }
    return count;
}

std::vector<std::string> SimulationStateAuditor::DeclaredBlocks(
    const std::string& systemName) const {
    std::vector<std::string> blocks;
    for (const auto& d : m_declarations) {
        if (d.systemName == systemName) {
            blocks.push_back(d.blockName);
        }
    }
    return blocks;
}

AuditResult SimulationStateAuditor::Audit() const {
    AuditResult result;
    result.systemsAudited = static_cast<uint32_t>(m_systems.size());
    result.stateBlocksVerified = static_cast<uint32_t>(m_declarations.size());
    result.passed = true;

    for (const auto& sys : m_systems) {
        size_t count = DeclaredStateCount(sys);
        if (count == 0) {
            AuditFinding finding;
            finding.severity = AuditFinding::Severity::Error;
            finding.systemName = sys;
            finding.message = "System '" + sys +
                "' has no declared state blocks — possible hidden state";
            result.findings.push_back(finding);
            result.errorsFound++;
            result.passed = false;
        }
    }

    // Check for declarations referencing unregistered systems
    for (const auto& decl : m_declarations) {
        bool found = false;
        for (const auto& sys : m_systems) {
            if (sys == decl.systemName) { found = true; break; }
        }
        if (!found) {
            AuditFinding finding;
            finding.severity = AuditFinding::Severity::Warning;
            finding.systemName = decl.systemName;
            finding.message = "State block '" + decl.blockName +
                "' declared for unregistered system '" + decl.systemName + "'";
            result.findings.push_back(finding);
            result.warningsFound++;
        }
    }

    return result;
}

void SimulationStateAuditor::Clear() {
    m_systems.clear();
    m_declarations.clear();
}

}  // namespace atlas::sim
