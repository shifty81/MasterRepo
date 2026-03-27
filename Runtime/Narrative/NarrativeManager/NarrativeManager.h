#pragma once
/**
 * @file NarrativeManager.h
 * @brief Story beat graph, branching narrative state machine, journal, and flag registry.
 *
 * Features:
 *   - Beat graph: nodes with text, speaker, choices (choice → next beat id)
 *   - Flag registry: SetFlag(name, value) / GetFlag(name) for world-state conditions
 *   - Condition evaluation: beat visibility gated on flag expressions (flag==val)
 *   - StartBeat(id): set current beat, fire OnBeatEnter callback
 *   - Choose(choiceIndex): advance to next beat, fire OnBeatExit/OnBeatEnter
 *   - Journal: auto-appends completed beats with timestamp
 *   - GetCurrentBeat() / GetChoices() accessors
 *   - LoadFromJSON(json) / SaveToJSON() for content pipeline
 *   - Reset() clears flags and journal, returns to start beat
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct NarrativeChoice {
    std::string text;        ///< display text
    std::string nextBeatId;  ///< target beat id, empty = end
    std::string condition;   ///< optional "flagName==value" predicate
};

struct NarrativeBeat {
    std::string id;
    std::string speaker;
    std::string body;
    std::vector<NarrativeChoice> choices;
    bool journalEntry{false}; ///< append to journal when visited
};

class NarrativeManager {
public:
    NarrativeManager();
    ~NarrativeManager();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Beat graph authoring
    void AddBeat  (const NarrativeBeat& beat);
    void RemoveBeat(const std::string& id);

    // Playback
    void StartBeat(const std::string& id);
    void Choose   (uint32_t choiceIndex);
    bool IsAtEnd  () const; ///< no current beat or no valid choices

    // Accessors
    const NarrativeBeat*                 GetCurrentBeat()     const;
    std::vector<const NarrativeChoice*>  GetAvailableChoices() const;
    const std::string&                   GetCurrentBeatId()   const;

    // Flags
    void        SetFlag (const std::string& name, const std::string& value);
    std::string GetFlag (const std::string& name) const;
    bool        HasFlag (const std::string& name) const;
    void        ClearFlag(const std::string& name);

    // Journal
    struct JournalEntry { std::string beatId; std::string body; uint32_t tick; };
    const std::vector<JournalEntry>& GetJournal() const;
    void ClearJournal();

    // Tick (increments internal timestamp for journal ordering)
    void Tick(float dt);

    // Callbacks
    void SetOnBeatEnter(std::function<void(const NarrativeBeat&)> cb);
    void SetOnBeatExit (std::function<void(const std::string& id)> cb);

    // Serialisation
    std::string SaveToJSON () const;
    bool        LoadFromJSON(const std::string& json);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
