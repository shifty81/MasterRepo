#pragma once
/**
 * @file StoryGenerator.h
 * @brief Procedural narrative generator for world events and quest arcs.
 *
 * StoryGenerator takes a world context (active quests, completed events,
 * faction states, player actions) and weaves them into a coherent Story Arc
 * with acts, beats, and a final resolution.  Output is a sequence of
 * NarrativeBeats that can drive dialogue, quest reveals, and world changes.
 *
 * Uses template-based beat patterns to ensure plausible narrative structure.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace PCG {

// ── Narrative primitives ────────────────────────────────────────────────────
enum class BeatType {
    Intro,          ///< Scene-setting
    IncitingEvent,  ///< Something disrupts the status quo
    RisingAction,   ///< Tension increases
    Climax,         ///< Peak conflict
    FallingAction,  ///< Aftermath
    Resolution,     ///< New status quo
    SideQuest,      ///< Optional branch
};

struct NarrativeBeat {
    uint32_t    id{0};
    BeatType    type{BeatType::Intro};
    std::string title;
    std::string description;   ///< Flavour text / quest overview
    std::string questID;       ///< Associated PCG::Quest id (empty if none)
    std::string locationID;
    std::string factionID;
    float       tension{0.0f}; ///< Narrative tension 0–1
};

struct StoryArc {
    uint32_t                  id{0};
    std::string               title;
    std::string               theme;         ///< e.g. "survival", "revenge"
    std::vector<NarrativeBeat> beats;
    float                     completionRatio{0.0f}; ///< Progress 0–1
};

// ── World context fed to the generator ───────────────────────────────────────
struct WorldContext {
    std::vector<std::string> completedQuestIDs;
    std::vector<std::string> activeQuestIDs;
    std::vector<std::string> activeFactionIDs;
    std::string              currentLocationID;
    int32_t                  playerLevel{1};
    uint64_t                 seed{0};          ///< 0 = use time
};

using BeatCallback = std::function<void(const NarrativeBeat&)>;

/// StoryGenerator — procedural narrative arc generator.
class StoryGenerator {
public:
    StoryGenerator();
    ~StoryGenerator();

    // ── themes ────────────────────────────────────────────────
    void RegisterTheme(const std::string& theme);
    size_t ThemeCount() const;

    // ── generation ────────────────────────────────────────────
    /// Generate a complete story arc from a world context.
    StoryArc Generate(const WorldContext& ctx);

    /// Generate and stream beats as they are created.
    StoryArc GenerateWithCallback(const WorldContext& ctx, BeatCallback cb);

    // ── progression ───────────────────────────────────────────
    /// Mark a beat as completed; updates arc completionRatio.
    bool CompleteBeat(StoryArc& arc, uint32_t beatId);

    /// Returns the next pending beat (lowest incomplete tension).
    const NarrativeBeat* NextBeat(const StoryArc& arc) const;

    // ── query ─────────────────────────────────────────────────
    std::vector<std::string> AvailableThemes() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace PCG
