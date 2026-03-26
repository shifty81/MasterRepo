#pragma once
/**
 * @file SceneStreamingSystem.h
 * @brief Asynchronous level / chunk streaming for open-world games.
 *
 * The world is divided into rectangular cells (chunks).  The streaming
 * system loads / unloads chunks based on the player's position and a
 * configurable load radius.  Loading is performed on a background thread
 * so the main thread is never blocked.
 *
 * Features:
 *   - Cell registration with asset path
 *   - Priority queue: cells nearest the player load first
 *   - Soft unload (fade-out) before hard unload
 *   - Event callbacks for game logic (spawn NPCs, run scripts)
 *   - Memory budget: unload distant cells when budget is exceeded
 *
 * Typical usage:
 * @code
 *   SceneStreamingSystem sss;
 *   StreamingConfig cfg;
 *   cfg.cellSize    = 100.f;
 *   cfg.loadRadius  = 300.f;
 *   cfg.unloadRadius= 500.f;
 *   sss.Init(cfg);
 *   sss.RegisterCell({0,0}, "World/Cells/cell_0_0.json");
 *   sss.SetPlayerPosition({50.f, 0.f, 50.f});
 *   // Each frame:
 *   sss.Update(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct CellCoord { int32_t x{0}, z{0}; bool operator==(const CellCoord& o) const{ return x==o.x&&z==o.z; } };

struct StreamingConfig {
    float    cellSize{100.f};
    float    loadRadius{250.f};
    float    unloadRadius{450.f};
    uint64_t memoryBudgetMB{512};
    uint32_t maxConcurrentLoads{4};
    bool     softUnload{true};
    float    softUnloadTime{2.f};
};

enum class CellState : uint8_t { Unloaded=0, Loading, Loaded, Unloading };

struct StreamingCell {
    CellCoord   coord;
    std::string assetPath;
    CellState   state{CellState::Unloaded};
    float       distToPlayer{0.f};
    uint64_t    memoryUsageMB{0};
    float       unloadAlpha{1.f};  ///< 1 = fully visible, 0 = faded
};

class SceneStreamingSystem {
public:
    SceneStreamingSystem();
    ~SceneStreamingSystem();

    void Init(const StreamingConfig& config = {});
    void Shutdown();

    // Cell management
    void RegisterCell(CellCoord coord, const std::string& assetPath);
    void UnregisterCell(CellCoord coord);
    bool HasCell(CellCoord coord) const;
    StreamingCell GetCell(CellCoord coord) const;
    std::vector<StreamingCell> AllCells() const;
    std::vector<StreamingCell> LoadedCells() const;

    // Player position drives load/unload decisions
    void SetPlayerPosition(const float worldPos[3]);

    // Manual overrides
    void ForceLoad(CellCoord coord);
    void ForceUnload(CellCoord coord);
    void ForceLoadAll();
    void UnloadAll();

    // Update: starts/finishes async load jobs
    void Update(float dt);

    // Stats
    uint64_t TotalLoadedMemoryMB() const;
    uint32_t LoadedCellCount()     const;
    uint32_t PendingLoadCount()    const;

    // Callbacks
    void OnCellLoaded(std::function<void(CellCoord, const std::string& assetPath)> cb);
    void OnCellUnloaded(std::function<void(CellCoord)> cb);
    void OnMemoryBudgetExceeded(std::function<void(uint64_t usedMB, uint64_t budgetMB)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Runtime
