#include "Core/TransactionManager/TransactionManager.h"
#include <stdexcept>
#include <stack>

namespace Core {

// ── Impl ─────────────────────────────────────────────────────────────────────
struct Frame {
    std::string               label;
    std::vector<StagedChange> staged;
};

struct TransactionManager::Impl {
    std::stack<Frame>           stack;
    TransactionStatus           status{TransactionStatus::Idle};
    std::vector<CommitCallback>   onCommit;
    std::vector<RollbackCallback> onRollback;
};

TransactionManager::TransactionManager() : m_impl(new Impl()) {}
TransactionManager::~TransactionManager() { delete m_impl; }

void TransactionManager::Begin(const std::string& label) {
    Frame f;
    f.label = label;
    m_impl->stack.push(std::move(f));
    m_impl->status = TransactionStatus::Active;
}

bool TransactionManager::Commit() {
    if (m_impl->stack.empty()) return false;

    Frame top = std::move(m_impl->stack.top());
    m_impl->stack.pop();

    if (!m_impl->stack.empty()) {
        // Nested: merge into parent frame.
        auto& parent = m_impl->stack.top();
        parent.staged.insert(parent.staged.end(),
                             top.staged.begin(), top.staged.end());
    } else {
        // Top-level commit.
        m_impl->status = TransactionStatus::Committed;
        for (auto& cb : m_impl->onCommit) cb(top.staged);
        m_impl->status = TransactionStatus::Idle;
    }
    return true;
}

void TransactionManager::Rollback() {
    if (m_impl->stack.empty()) return;
    Frame top = std::move(m_impl->stack.top());
    m_impl->stack.pop();
    m_impl->status = TransactionStatus::RolledBack;
    for (auto& cb : m_impl->onRollback) cb(top.staged);
    if (m_impl->stack.empty())
        m_impl->status = TransactionStatus::Idle;
    else
        m_impl->status = TransactionStatus::Active;
}

void TransactionManager::Stage(const StagedChange& change) {
    if (m_impl->stack.empty()) return;
    m_impl->stack.top().staged.push_back(change);
}

void TransactionManager::StageAll(const std::vector<StagedChange>& changes) {
    if (m_impl->stack.empty()) return;
    auto& staged = m_impl->stack.top().staged;
    staged.insert(staged.end(), changes.begin(), changes.end());
}

TransactionStatus TransactionManager::Status() const { return m_impl->status; }

size_t TransactionManager::StagedCount() const {
    return m_impl->stack.empty() ? 0 : m_impl->stack.top().staged.size();
}

std::vector<StagedChange> TransactionManager::GetStaged() const {
    return m_impl->stack.empty()
               ? std::vector<StagedChange>{}
               : m_impl->stack.top().staged;
}

std::string TransactionManager::CurrentLabel() const {
    return m_impl->stack.empty() ? "" : m_impl->stack.top().label;
}

int TransactionManager::Depth() const {
    return static_cast<int>(m_impl->stack.size());
}

void TransactionManager::OnCommit(CommitCallback cb) {
    m_impl->onCommit.push_back(std::move(cb));
}
void TransactionManager::OnRollback(RollbackCallback cb) {
    m_impl->onRollback.push_back(std::move(cb));
}

} // namespace Core
