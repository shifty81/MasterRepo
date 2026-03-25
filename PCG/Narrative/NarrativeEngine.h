#pragma once
/**
 * @file NarrativeEngine.h
 * @brief Procedural narrative — dynamic dialogue driven by player actions, reputation, and quests.
 *
 * Manages:
 *   - NarrativeContext: faction reputation, active quests, player stats, history flags
 *   - DialogueTemplate: parameterised text with condition predicates
 *   - NarrativeEvent: discrete story beats (first landing, faction war, boss kill, etc.)
 *   - StoryArc: ordered sequence of NarrativeEvents with branching
 *   - Dynamic text generation: substitute context variables into templates
 *   - Event-driven: hook into FactionSystem, CombatSystem, ProgressionSystem
 *
 * Usage:
 * @code
 *   NarrativeEngine narrative;
 *   narrative.Init();
 *   narrative.SetContext("faction_miners_rep", 300.0f);
 *   narrative.SetFlag("completed_first_delivery");
 *   auto line = narrative.GenerateLine("npc_miner_greeting");
 *   // → "Good to see you again, Commander! The guild appreciates your help."
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace PCG {

// ── Narrative context ─────────────────────────────────────────────────────────

struct NarrativeContext {
    std::unordered_map<std::string, float>       numbers;  ///< faction rep, XP, etc.
    std::unordered_map<std::string, std::string> strings;  ///< player name, ship name
    std::unordered_set<std::string>              flags;    ///< boolean event flags
};

// ── Dialogue template ─────────────────────────────────────────────────────────

struct DialogueVariant {
    std::string text;                                   ///< may contain {variable} tokens
    std::function<bool(const NarrativeContext&)> condition; ///< nullptr = always eligible
    float       weight{1.0f};                           ///< relative probability
};

struct DialogueTemplate {
    std::string                 id;
    std::vector<DialogueVariant> variants;
};

// ── Narrative event ───────────────────────────────────────────────────────────

struct NarrativeEvent {
    uint32_t    id{0};
    std::string name;
    std::string description;
    std::function<bool(const NarrativeContext&)> trigger;
    bool        fired{false};
    bool        repeatable{false};
    std::vector<std::string> setsFlags;      ///< flags to set when fired
    std::vector<std::string> clearFlags;     ///< flags to clear when fired
};

// ── Story arc ─────────────────────────────────────────────────────────────────

struct StoryArc {
    uint32_t                   id{0};
    std::string                name;
    std::vector<uint32_t>      eventIds;     ///< ordered
    size_t                     progress{0};  ///< index of next event
    bool                       complete{false};
};

using NarrativeEventFiredFn = std::function<void(const NarrativeEvent&)>;
using ArcCompletedFn        = std::function<void(const StoryArc&)>;

// ── NarrativeEngine ───────────────────────────────────────────────────────────

class NarrativeEngine {
public:
    void Init();

    // ── Context management ────────────────────────────────────────────────
    void  SetContext(const std::string& key, float value);
    void  SetContext(const std::string& key, const std::string& value);
    void  SetFlag(const std::string& flag);
    void  ClearFlag(const std::string& flag);
    bool  HasFlag(const std::string& flag) const;
    float GetNumber(const std::string& key, float defaultVal = 0.0f) const;
    const NarrativeContext& GetContext() const;

    // ── Dialogue ──────────────────────────────────────────────────────────
    void  AddDialogueTemplate(const DialogueTemplate& tmpl);

    /// Generate a contextual line for the given template id.
    /// Returns empty string if no eligible variant.
    std::string GenerateLine(const std::string& templateId) const;

    /// Generate a line with extra overrides applied temporarily.
    std::string GenerateLine(const std::string& templateId,
                              const NarrativeContext& overrides) const;

    // ── Events ────────────────────────────────────────────────────────────
    uint32_t AddEvent(NarrativeEvent evt);
    void     FireEvent(uint32_t eventId);

    /// Evaluate all event triggers against the current context.
    /// Fires any triggered events automatically.
    void EvaluateEvents();

    // ── Story arcs ────────────────────────────────────────────────────────
    uint32_t  AddStoryArc(const std::string& name,
                           const std::vector<uint32_t>& eventIds);
    StoryArc* GetStoryArc(uint32_t arcId);
    void      AdvanceArc(uint32_t arcId);
    bool      IsArcComplete(uint32_t arcId) const;

    // ── Tick ──────────────────────────────────────────────────────────────
    /// Call each game tick to automatically evaluate events / advance arcs.
    void Tick(float dt);

    // ── Callbacks ─────────────────────────────────────────────────────────
    void SetEventFiredCallback(NarrativeEventFiredFn fn);
    void SetArcCompletedCallback(ArcCompletedFn fn);

    // ── Stats ─────────────────────────────────────────────────────────────
    size_t DialogueTemplateCount() const;
    size_t EventCount()            const;
    size_t ArcCount()              const;

private:
    std::string Substitute(const std::string& text,
                            const NarrativeContext& ctx) const;
    std::string SubstituteCtx(const std::string& text) const;

    NarrativeContext m_ctx;
    std::unordered_map<std::string, DialogueTemplate> m_templates;
    std::unordered_map<uint32_t, NarrativeEvent>      m_events;
    std::unordered_map<uint32_t, StoryArc>            m_arcs;
    uint32_t m_nextEventId{1};
    uint32_t m_nextArcId{1};

    NarrativeEventFiredFn m_onEventFired;
    ArcCompletedFn        m_onArcCompleted;
};

} // namespace PCG
