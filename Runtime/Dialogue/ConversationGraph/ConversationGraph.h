#pragma once
/**
 * @file ConversationGraph.h
 * @brief Node-graph–based conversation / dialogue system.
 *
 * A ConversationGraph is a directed graph of DialogueNodes connected by
 * Choices or conditions.  The runtime evaluator advances through the graph
 * one node at a time, evaluating conditions and emitting lines.
 *
 * Features:
 *   - Speaker / listener assignment per node
 *   - Condition predicates (variable comparisons, quest flags)
 *   - Side effects (set variable, trigger event, play animation)
 *   - Branching choices with prerequisites
 *   - Localisation key support (each line has a key + fallback text)
 *   - JSON serialization
 *
 * Typical usage:
 * @code
 *   ConversationGraph g;
 *   g.LoadFromFile("Dialogue/blacksmith_intro.json");
 *   g.Start();
 *   while (g.IsActive()) {
 *       auto node = g.CurrentNode();
 *       Display(node.text);
 *       auto choices = g.GetChoices();
 *       g.Choose(choices[playerChoice].id);
 *   }
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct DialogueCondition {
    std::string variable;
    std::string op;        ///< "==", "!=", ">", "<", ">="
    std::string value;
};

struct DialogueEffect {
    std::string type;      ///< "set_var", "trigger_event", "play_anim"
    std::string target;
    std::string value;
};

struct DialogueChoice {
    uint32_t    id{0};
    std::string text;
    std::string locKey;
    uint32_t    nextNodeId{0};
    std::vector<DialogueCondition> prerequisites;
};

struct DialogueNode {
    uint32_t    id{0};
    std::string speaker;
    std::string text;
    std::string locKey;
    std::string voiceClip;          ///< optional audio asset path
    std::string animation;          ///< optional anim name
    std::vector<DialogueChoice> choices;
    std::vector<DialogueEffect> effects;
    uint32_t    autoNextId{0};      ///< if no choices, auto-advance here
    float       autoAdvanceDelay{0.f};
};

enum class ConversationState : uint8_t { Idle=0, Active, WaitingChoice, Finished };

class ConversationGraph {
public:
    ConversationGraph();
    ~ConversationGraph();

    bool LoadFromFile(const std::string& path);
    bool SaveToFile(const std::string& path) const;
    void Clear();

    // Node authoring
    uint32_t   AddNode(const DialogueNode& node);
    void       UpdateNode(const DialogueNode& node);
    void       RemoveNode(uint32_t id);
    DialogueNode GetNode(uint32_t id) const;
    std::vector<DialogueNode> AllNodes() const;

    // Runtime
    void Start(uint32_t startNodeId = 0);
    void Stop();
    bool IsActive() const;

    DialogueNode    CurrentNode() const;
    std::vector<DialogueChoice> GetChoices() const;
    bool            Choose(uint32_t choiceId);
    void            Advance();          ///< for auto-advance nodes
    void            Tick(float dt);
    ConversationState GetState() const;

    // Variable store for conditions
    void  SetVariable(const std::string& key, const std::string& value);
    std::string GetVariable(const std::string& key) const;

    // Callbacks
    void OnNodeEnter(std::function<void(const DialogueNode&)> cb);
    void OnChoiceAvailable(std::function<void(const std::vector<DialogueChoice>&)> cb);
    void OnFinished(std::function<void()> cb);
    void OnEffect(std::function<void(const DialogueEffect&)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Runtime
