#include "AI/SuggestionSandbox/SuggestionSandbox.h"
#include <chrono>
#include <stack>
#include <unordered_map>
#include <vector>

namespace AI {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

struct SuggestionSandbox::Impl {
    std::unordered_map<uint64_t, StagedSuggestion>   suggestions;
    std::stack<uint64_t>                              undoStack; // committed ids
    std::stack<uint64_t>                              redoStack;
    uint64_t                                          nextId{1};

    std::function<void(const StagedSuggestion&)> onCommit;
    std::function<void(const StagedSuggestion&)> onRollback;
    std::function<void(const StagedSuggestion&)> onReject;
};

SuggestionSandbox::SuggestionSandbox() : m_impl(new Impl()) {}
SuggestionSandbox::~SuggestionSandbox() { delete m_impl; }

void SuggestionSandbox::Init()     {}
void SuggestionSandbox::Shutdown() { Clear(); }

uint64_t SuggestionSandbox::Stage(const std::string& tag,
                                   const std::string& description,
                                   float confidence) {
    uint64_t id = m_impl->nextId++;
    StagedSuggestion& s = m_impl->suggestions[id];
    s.id          = id;
    s.tag         = tag;
    s.description = description;
    s.confidence  = confidence;
    s.state       = SuggestionState::Staged;
    s.createdMs   = NowMs();
    return id;
}

void SuggestionSandbox::SetPayload(uint64_t id, const std::string& payload) {
    auto it = m_impl->suggestions.find(id);
    if (it != m_impl->suggestions.end()) it->second.payload = payload;
}

void SuggestionSandbox::Preview(uint64_t id) {
    auto it = m_impl->suggestions.find(id);
    if (it != m_impl->suggestions.end())
        it->second.state = SuggestionState::Previewed;
}

void SuggestionSandbox::Commit(uint64_t id) {
    auto it = m_impl->suggestions.find(id);
    if (it == m_impl->suggestions.end()) return;
    it->second.state       = SuggestionState::Committed;
    it->second.committedMs = NowMs();
    // clear redo when a new commit happens
    while (!m_impl->redoStack.empty()) m_impl->redoStack.pop();
    m_impl->undoStack.push(id);
    if (m_impl->onCommit) m_impl->onCommit(it->second);
}

void SuggestionSandbox::Reject(uint64_t id) {
    auto it = m_impl->suggestions.find(id);
    if (it == m_impl->suggestions.end()) return;
    it->second.state = SuggestionState::Rejected;
    if (m_impl->onReject) m_impl->onReject(it->second);
}

bool SuggestionSandbox::CanUndo()  const { return !m_impl->undoStack.empty(); }
bool SuggestionSandbox::CanRedo()  const { return !m_impl->redoStack.empty(); }

void SuggestionSandbox::Undo() {
    if (m_impl->undoStack.empty()) return;
    uint64_t id = m_impl->undoStack.top(); m_impl->undoStack.pop();
    auto it = m_impl->suggestions.find(id);
    if (it != m_impl->suggestions.end()) {
        it->second.state = SuggestionState::RolledBack;
        m_impl->redoStack.push(id);
        if (m_impl->onRollback) m_impl->onRollback(it->second);
    }
}

void SuggestionSandbox::Redo() {
    if (m_impl->redoStack.empty()) return;
    uint64_t id = m_impl->redoStack.top(); m_impl->redoStack.pop();
    auto it = m_impl->suggestions.find(id);
    if (it != m_impl->suggestions.end()) {
        it->second.state       = SuggestionState::Committed;
        it->second.committedMs = NowMs();
        m_impl->undoStack.push(id);
        if (m_impl->onCommit) m_impl->onCommit(it->second);
    }
}

uint32_t SuggestionSandbox::UndoDepth() const {
    return static_cast<uint32_t>(m_impl->undoStack.size());
}

StagedSuggestion SuggestionSandbox::GetSuggestion(uint64_t id) const {
    auto it = m_impl->suggestions.find(id);
    if (it == m_impl->suggestions.end()) return {};
    return it->second;
}

std::vector<StagedSuggestion> SuggestionSandbox::AllSuggestions() const {
    std::vector<StagedSuggestion> out;
    out.reserve(m_impl->suggestions.size());
    for (auto& [k, v] : m_impl->suggestions) out.push_back(v);
    return out;
}

std::vector<StagedSuggestion> SuggestionSandbox::StagedSuggestions() const {
    std::vector<StagedSuggestion> out;
    for (auto& [k, v] : m_impl->suggestions)
        if (v.state == SuggestionState::Staged ||
            v.state == SuggestionState::Previewed) out.push_back(v);
    return out;
}

std::vector<StagedSuggestion> SuggestionSandbox::CommittedSuggestions() const {
    std::vector<StagedSuggestion> out;
    for (auto& [k, v] : m_impl->suggestions)
        if (v.state == SuggestionState::Committed) out.push_back(v);
    return out;
}

void SuggestionSandbox::OnCommit(std::function<void(const StagedSuggestion&)> cb) {
    m_impl->onCommit = std::move(cb);
}

void SuggestionSandbox::OnRollback(std::function<void(const StagedSuggestion&)> cb) {
    m_impl->onRollback = std::move(cb);
}

void SuggestionSandbox::OnReject(std::function<void(const StagedSuggestion&)> cb) {
    m_impl->onReject = std::move(cb);
}

void SuggestionSandbox::Clear() {
    m_impl->suggestions.clear();
    while (!m_impl->undoStack.empty()) m_impl->undoStack.pop();
    while (!m_impl->redoStack.empty()) m_impl->redoStack.pop();
}

} // namespace AI
