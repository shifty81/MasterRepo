#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace Runtime {

/// A single line of dialogue (speaker text + optional choices).
struct DialogueLine {
    std::string speakerId;  ///< NPC or player id / name
    std::string text;       ///< Dialogue text
    uint32_t    nodeId{0};  ///< ID of this node in the tree
    std::vector<uint32_t> nextNodeIds; ///< Possible follow-on nodes (branches)
    std::string condition;  ///< Optional script/flag condition string
    std::string action;     ///< Optional side-effect action tag
};

/// A full conversation tree.
struct DialogueTree {
    std::string              id;
    uint32_t                 rootNodeId{0};
    std::vector<DialogueLine> nodes;
};

struct DialogueEvent {
    std::string  treeId;
    uint32_t     npcId{0};
    uint32_t     currentNodeId{0};
    std::string  speakerId;
    std::string  text;
    bool         isEnd{false};
};

using DialogueEventCallback = std::function<void(const DialogueEvent&)>;
using ChoiceCallback        = std::function<void(uint32_t nodeId)>; ///< Player selects a branch

/// DialogueSystem — runtime dialogue tree engine for NPC conversations.
///
/// Conversation trees are registered at startup; the runtime queries
/// the active node via AdvanceTo() or SelectChoice().  NPCController
/// can trigger conversations by calling StartDialogue(treeId, npcId).
class DialogueSystem {
public:
    DialogueSystem();
    ~DialogueSystem();

    // ── registration ──────────────────────────────────────────
    void RegisterTree(const DialogueTree& tree);
    bool HasTree(const std::string& treeId) const;
    void ClearAll();

    // ── session control ───────────────────────────────────────
    /// Begin a conversation.
    bool StartDialogue(const std::string& treeId, uint32_t npcId);
    /// Move to next sequential node (single-branch conversations).
    bool Advance();
    /// Player selects a branching choice.
    bool SelectChoice(uint32_t choiceNodeId);
    /// End the current conversation.
    void End();
    bool IsActive() const;

    // ── query ─────────────────────────────────────────────────
    const DialogueLine* CurrentLine() const;
    /// Returns all valid choice nodes for the current line.
    std::vector<const DialogueLine*> GetChoices() const;

    // ── callbacks ─────────────────────────────────────────────
    void OnDialogueEvent(DialogueEventCallback cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
