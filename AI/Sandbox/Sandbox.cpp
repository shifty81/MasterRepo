#include "AI/Sandbox/Sandbox.h"
#include <algorithm>
#include <fstream>

namespace AI {

uint64_t Sandbox::StageChange(SandboxChange change) {
    change.id = m_nextId++;
    change.approved = false;
    m_pending.push_back(change);
    return change.id;
}

void Sandbox::ApproveChange(uint64_t id) {
    for (auto& c : m_pending) if (c.id == id) { c.approved = true; return; }
}

void Sandbox::RejectChange(uint64_t id) {
    auto it = std::remove_if(m_pending.begin(), m_pending.end(),
        [id](const SandboxChange& c){ return c.id == id; });
    m_pending.erase(it, m_pending.end());
}

void Sandbox::ApproveAll() { for (auto& c : m_pending) c.approved = true; }

void Sandbox::RejectAll() {
    m_pending.erase(std::remove_if(m_pending.begin(), m_pending.end(),
        [](const SandboxChange& c){ return !c.approved; }), m_pending.end());
}

bool Sandbox::Apply(uint64_t id) {
    for (auto it = m_pending.begin(); it != m_pending.end(); ++it) {
        if (it->id == id && it->approved) {
            bool ok = applyChange(*it);
            if (ok) {
                m_applied.push_back(*it);
                m_pending.erase(it);
            }
            return ok;
        }
    }
    return false;
}

size_t Sandbox::ApplyApproved() {
    size_t count = 0;
    for (auto it = m_pending.begin(); it != m_pending.end(); ) {
        if (it->approved && applyChange(*it)) {
            m_applied.push_back(*it);
            it = m_pending.erase(it);
            ++count;
        } else {
            ++it;
        }
    }
    return count;
}

std::vector<SandboxChange> Sandbox::GetPending() const { return m_pending; }
std::vector<SandboxChange> Sandbox::GetApplied() const { return m_applied; }
size_t Sandbox::Count() const { return m_pending.size(); }
void Sandbox::Clear() { m_pending.clear(); }

bool Sandbox::applyChange(const SandboxChange& change) {
    switch (change.action) {
        case SandboxAction::CreateFile:
        case SandboxAction::ModifyFile: {
            std::ofstream f(change.path);
            if (!f) return false;
            f << change.newContent;
            return true;
        }
        case SandboxAction::DeleteFile:
            return std::remove(change.path.c_str()) == 0;
        case SandboxAction::RunCommand:
            // Commands are not auto-executed for safety
            return false;
    }
    return false;
}

} // namespace AI
