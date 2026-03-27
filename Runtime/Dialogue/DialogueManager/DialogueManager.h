#pragma once
/**
 * @file DialogueManager.h
 * @brief Conversation flow manager: lines, choices, branches, and flag conditions.
 *
 * Features:
 *   - Conversations defined as directed graphs (nodes with speaker/text/choices)
 *   - Choice branches: each choice leads to a target node id
 *   - Condition gates: choices or nodes only visible when a flag is set
 *   - Flag system: set/clear/query boolean flags (integrates with game state)
 *   - Auto-advance nodes (no choice; goes to next node after optional duration)
 *   - Variable substitution in text ("{player_name}", "{score}")
 *   - On-enter / on-exit node callbacks
 *   - On-choice-selected callback
 *   - JSON conversation import
 *   - Multiple concurrent conversations (one per NPC pair)
 *
 * Typical usage:
 * @code
 *   DialogueManager dm;
 *   dm.Init();
 *   dm.LoadConversation("Data/Dialogue/npc_guard.json");
 *   uint32_t convId = dm.StartConversation("npc_guard", playerId, npcId);
 *   dm.SetOnNodeEnter(convId, [](const DialogueNode& n){ ShowDialogueUI(n); });
 *   dm.SetOnChoicesAvailable(convId, [](auto& choices){ ShowChoiceUI(choices); });
 *   dm.SelectChoice(convId, 0);  // player picks first option
 *   dm.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct DialogueChoice {
    uint32_t    choiceIndex{0};
    std::string text;
    std::string targetNodeId;
    std::string conditionFlag;  ///< only shown when this flag is set ("" = always)
    bool        available{true};
};

struct DialogueNode {
    std::string                 nodeId;
    std::string                 speaker;    ///< actor name or "player"
    std::string                 text;       ///< may contain {variable} tokens
    std::vector<DialogueChoice> choices;
    std::string                 nextNodeId; ///< auto-advance if no choices
    float                       autoAdvanceDuration{0.f}; ///< 0 = manual advance
    std::string                 onEnterFlag;  ///< flag to set on node enter
    std::string                 onExitFlag;   ///< flag to set on node exit
};

struct ConversationDef {
    std::string                               id;
    std::string                               startNodeId;
    std::unordered_map<std::string, DialogueNode> nodes;
};

struct ActiveConversation {
    uint32_t    convId{0};
    std::string defId;
    uint32_t    participantA{0};
    uint32_t    participantB{0};
    std::string currentNodeId;
    float       autoTimer{0.f};
    bool        waitingForChoice{false};
    bool        finished{false};
};

class DialogueManager {
public:
    DialogueManager();
    ~DialogueManager();

    void Init();
    void Shutdown();
    void Tick(float dt);

    // Conversation definitions
    void RegisterConversation(const ConversationDef& def);
    bool LoadConversation(const std::string& jsonPath);
    bool HasConversation(const std::string& defId) const;

    // Runtime sessions
    uint32_t StartConversation(const std::string& defId,
                                uint32_t actorA, uint32_t actorB=0);
    void     EndConversation(uint32_t convId);
    bool     IsActive(uint32_t convId) const;

    // Advance
    void AdvanceLine   (uint32_t convId);        ///< move to nextNodeId
    void SelectChoice  (uint32_t convId, uint32_t choiceIndex);
    std::vector<DialogueChoice> GetChoices(uint32_t convId) const;
    const DialogueNode* GetCurrentNode(uint32_t convId) const;

    // Flags
    void SetFlag  (const std::string& flag);
    void ClearFlag(const std::string& flag);
    bool HasFlag  (const std::string& flag) const;

    // Variable substitution table
    void SetVariable(const std::string& key, const std::string& value);
    std::string ResolveText(const std::string& text) const;

    // Callbacks
    using NodeCb    = std::function<void(const DialogueNode&)>;
    using ChoiceCb  = std::function<void(const std::vector<DialogueChoice>&)>;
    using FinishCb  = std::function<void(uint32_t convId)>;
    void SetOnNodeEnter        (uint32_t convId, NodeCb   cb);
    void SetOnNodeExit         (uint32_t convId, NodeCb   cb);
    void SetOnChoicesAvailable (uint32_t convId, ChoiceCb cb);
    void SetOnConversationEnd  (uint32_t convId, FinishCb cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
