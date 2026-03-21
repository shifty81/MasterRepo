#include "Runtime/Dialogue/DialogueSystem.h"
#include <unordered_map>
#include <algorithm>

namespace Runtime {

struct DialogueSystem::Impl {
    std::unordered_map<std::string, DialogueTree> trees;

    // active session
    bool        active{false};
    std::string currentTreeId;
    uint32_t    currentNpcId{0};
    uint32_t    currentNodeId{0};

    std::vector<DialogueEventCallback> callbacks;
};

DialogueSystem::DialogueSystem() : m_impl(new Impl()) {}
DialogueSystem::~DialogueSystem() { delete m_impl; }

void DialogueSystem::RegisterTree(const DialogueTree& tree) {
    m_impl->trees[tree.id] = tree;
}

bool DialogueSystem::HasTree(const std::string& id) const {
    return m_impl->trees.count(id) > 0;
}

void DialogueSystem::ClearAll() { m_impl->trees.clear(); }

bool DialogueSystem::StartDialogue(const std::string& treeId, uint32_t npcId) {
    auto it = m_impl->trees.find(treeId);
    if (it == m_impl->trees.end()) return false;
    m_impl->active        = true;
    m_impl->currentTreeId = treeId;
    m_impl->currentNpcId  = npcId;
    m_impl->currentNodeId = it->second.rootNodeId;

    const DialogueLine* line = CurrentLine();
    if (line) {
        DialogueEvent ev{treeId, npcId, line->nodeId, line->speakerId, line->text, false};
        for (auto& cb : m_impl->callbacks) cb(ev);
    }
    return true;
}

bool DialogueSystem::Advance() {
    if (!m_impl->active) return false;
    const DialogueLine* line = CurrentLine();
    if (!line || line->nextNodeIds.empty()) {
        End();
        return false;
    }
    // Take the first (or only) branch.
    return SelectChoice(line->nextNodeIds[0]);
}

bool DialogueSystem::SelectChoice(uint32_t choiceNodeId) {
    if (!m_impl->active) return false;
    auto& tree = m_impl->trees.at(m_impl->currentTreeId);

    // Validate that choiceNodeId is in nextNodeIds of current line.
    const DialogueLine* cur = CurrentLine();
    if (cur) {
        bool valid = std::find(cur->nextNodeIds.begin(),
                               cur->nextNodeIds.end(),
                               choiceNodeId) != cur->nextNodeIds.end();
        if (!valid) return false;
    }

    m_impl->currentNodeId = choiceNodeId;
    const DialogueLine* next = CurrentLine();
    if (!next) { End(); return false; }

    DialogueEvent ev{m_impl->currentTreeId, m_impl->currentNpcId,
                     next->nodeId, next->speakerId, next->text,
                     next->nextNodeIds.empty()};
    for (auto& cb : m_impl->callbacks) cb(ev);
    return true;
}

void DialogueSystem::End() {
    if (!m_impl->active) return;
    DialogueEvent ev{m_impl->currentTreeId, m_impl->currentNpcId,
                     m_impl->currentNodeId, "", "", true};
    for (auto& cb : m_impl->callbacks) cb(ev);
    m_impl->active = false;
}

bool DialogueSystem::IsActive() const { return m_impl->active; }

const DialogueLine* DialogueSystem::CurrentLine() const {
    if (!m_impl->active) return nullptr;
    auto it = m_impl->trees.find(m_impl->currentTreeId);
    if (it == m_impl->trees.end()) return nullptr;
    for (const auto& n : it->second.nodes)
        if (n.nodeId == m_impl->currentNodeId) return &n;
    return nullptr;
}

std::vector<const DialogueLine*> DialogueSystem::GetChoices() const {
    const DialogueLine* cur = CurrentLine();
    if (!cur) return {};
    auto& tree = m_impl->trees.at(m_impl->currentTreeId);
    std::vector<const DialogueLine*> choices;
    for (uint32_t nid : cur->nextNodeIds)
        for (const auto& n : tree.nodes)
            if (n.nodeId == nid) { choices.push_back(&n); break; }
    return choices;
}

void DialogueSystem::OnDialogueEvent(DialogueEventCallback cb) {
    m_impl->callbacks.push_back(std::move(cb));
}

} // namespace Runtime
