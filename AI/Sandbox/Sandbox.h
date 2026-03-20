#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace AI {

enum class SandboxAction : uint8_t {
    CreateFile,
    ModifyFile,
    DeleteFile,
    RunCommand,
};

struct SandboxChange {
    uint64_t      id         = 0;
    SandboxAction action     = SandboxAction::CreateFile;
    std::string   path;
    std::string   oldContent;
    std::string   newContent;
    bool          approved   = false;
};

class Sandbox {
public:
    uint64_t StageChange(SandboxChange change);
    void     ApproveChange(uint64_t id);
    void     RejectChange(uint64_t id);
    void     ApproveAll();
    void     RejectAll();
    bool     Apply(uint64_t id);
    size_t   ApplyApproved();
    std::vector<SandboxChange> GetPending() const;
    std::vector<SandboxChange> GetApplied() const;
    size_t   Count() const;
    void     Clear();

private:
    bool applyChange(const SandboxChange& change);

    std::vector<SandboxChange> m_pending;
    std::vector<SandboxChange> m_applied;
    uint64_t                   m_nextId = 1;
};

} // namespace AI
