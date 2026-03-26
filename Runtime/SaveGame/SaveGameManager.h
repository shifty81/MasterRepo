#pragma once
/**
 * @file SaveGameManager.h
 * @brief Structured save-game system with named slots, versioning, and
 *        automatic migration for format changes.
 *
 * Wraps the lower-level `Runtime/SaveLoad` module with:
 *   - Named save slots (slot 0–9 + named manual saves)
 *   - Save metadata (timestamp, playtime, screenshot path, description)
 *   - Schema version tracking and user-defined migration callbacks
 *   - Auto-save on interval or game event
 *   - Cloud-sync hook (delegates to `Core/CloudSync` if available)
 *
 * Typical usage:
 * @code
 *   SaveGameManager saves;
 *   saves.Init("Saves/");
 *   saves.SetMigration(1, 2, [](SaveData& d){ /* upgrade v1→v2 * / });
 *   saves.SaveSlot(0, gameState, "Camp Outpost");
 *   auto data = saves.LoadSlot(0);
 *   saves.EnableAutoSave(300.f);  // auto-save every 5 minutes
 *   saves.Tick(deltaSeconds);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

// ── Opaque save payload ───────────────────────────────────────────────────────

using SaveData = std::string;   ///< JSON-serialised game state blob

// ── Save slot metadata ────────────────────────────────────────────────────────

struct SaveSlotInfo {
    uint32_t    slot{0};
    std::string name;
    std::string description;
    uint64_t    timestampMs{0};
    uint64_t    playtimeSeconds{0};
    uint32_t    schemaVersion{1};
    std::string screenshotPath;
    bool        isEmpty{true};
};

// ── SaveGameManager ───────────────────────────────────────────────────────────

class SaveGameManager {
public:
    SaveGameManager();
    ~SaveGameManager();

    void Init(const std::string& saveDir = "Saves/");
    void Shutdown();

    // ── Save / Load ───────────────────────────────────────────────────────────

    bool     SaveSlot(uint32_t slot, const SaveData& data,
                      const std::string& description = "");
    SaveData LoadSlot(uint32_t slot);
    bool     DeleteSlot(uint32_t slot);
    bool     SlotExists(uint32_t slot) const;

    // Named saves (e.g. quicksave, autosave)
    bool     SaveNamed(const std::string& name, const SaveData& data);
    SaveData LoadNamed(const std::string& name);

    // ── Metadata & listing ────────────────────────────────────────────────────

    SaveSlotInfo              GetSlotInfo(uint32_t slot)   const;
    std::vector<SaveSlotInfo> ListSlots()                  const;
    std::vector<SaveSlotInfo> ListNamedSaves()             const;

    // ── Schema versioning & migration ─────────────────────────────────────────

    void SetCurrentSchemaVersion(uint32_t version);

    /// Register a migration from one schema version to the next.
    void SetMigration(uint32_t fromVersion, uint32_t toVersion,
                      std::function<void(SaveData&)> migrateFn);

    // ── Auto-save ─────────────────────────────────────────────────────────────

    void EnableAutoSave(float intervalSeconds, uint32_t slot = 0);
    void DisableAutoSave();

    /// Advance auto-save timer.  Fires auto-save via the callback when due.
    void Tick(float deltaSeconds);

    /// Called by the game to provide the current save data for auto-saves.
    void SetAutoSaveProvider(std::function<SaveData()> provider);

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnSaveComplete(std::function<void(uint32_t slot)> cb);
    void OnLoadComplete(std::function<void(uint32_t slot)> cb);
    void OnSaveError(std::function<void(const std::string& error)> cb);

    // ── Playtime tracking ─────────────────────────────────────────────────────

    void     SetPlaytimeSeconds(uint64_t seconds);
    uint64_t PlaytimeSeconds() const;

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Runtime
