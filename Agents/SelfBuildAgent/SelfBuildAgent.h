#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace Agents {

// ──────────────────────────────────────────────────────────────
// A single iteration produced by the self-build agent
// ──────────────────────────────────────────────────────────────

struct BuildIteration {
    std::string id;                // unique iteration ID
    std::string goal;              // original high-level goal
    std::string generatedCode;     // code produced in this iteration
    std::string assetDescription;  // description of generated assets
    std::string archivePath;       // path where iteration was archived
    bool        buildPassed = false;
    bool        testsPassed = false;
    std::string buildLog;
    std::string testLog;
    double      scoreHeuristic = 0.0; // higher is better
};

// ──────────────────────────────────────────────────────────────
// SelfBuildAgent — Phase 17 Offline Self-Build Automation
//
// Workflow:
//  1. User provides high-level goal string
//  2. Agent generates code / asset descriptions via AI
//  3. Agent validates via build + test pipeline
//  4. Agent archives each iteration to Archive/
//  5. User calls ReviewIterations() to approve / reject
//  6. Repeat until approved
// ──────────────────────────────────────────────────────────────

class SelfBuildAgent {
public:
    // --- Configuration ----------------------------------------------

    // Directory where iterations are archived (default: "Archive/SelfBuild")
    void SetArchiveDir(const std::string& dir);

    // Maximum number of auto-retry iterations before stopping
    void SetMaxIterations(int max);

    // Plug in the build command (e.g. "cmake --build /tmp/build")
    void SetBuildCommand(const std::string& cmd);
    void SetTestCommand(const std::string& cmd);

    // Plug in the code-generation callback (AI back-end)
    using GenerateFn = std::function<std::string(const std::string& goal,
                                                  const BuildIteration* lastIter)>;
    void SetGenerateCallback(GenerateFn fn);

    // Plug in the asset-generation callback (Blender / PCG)
    using AssetFn = std::function<std::string(const std::string& goal)>;
    void SetAssetCallback(AssetFn fn);

    // --- Execution --------------------------------------------------

    // Run one full self-build loop for the given goal; returns best iteration
    BuildIteration Run(const std::string& goal);

    // Run a single step and return the iteration (for interactive use)
    BuildIteration Step(const std::string& goal);

    // --- Review & Approval ------------------------------------------

    const std::vector<BuildIteration>& Iterations() const { return m_iterations; }

    // Approve the iteration at index (merges it into the live project)
    bool Approve(size_t index);

    // Reject the iteration at index; optionally provide feedback
    void Reject(size_t index, const std::string& feedback = "");

    // Clear all stored iterations (does NOT delete archives)
    void ClearIterations();

    // --- Callbacks --------------------------------------------------

    using IterationCallback = std::function<void(const BuildIteration&)>;
    void OnIterationComplete(IterationCallback cb);
    void OnApproved(IterationCallback cb);

private:
    BuildIteration  RunIteration(const std::string& goal, int attempt);
    bool            DoBuild(BuildIteration& iter);
    bool            DoTest (BuildIteration& iter);
    void            ArchiveIteration(BuildIteration& iter);
    std::string     GenerateIterationId(int attempt) const;

    std::string  m_archiveDir    = "Archive/SelfBuild";
    int          m_maxIterations = 5;
    std::string  m_buildCmd      = "cmake --build .";
    std::string  m_testCmd       = "ctest --output-on-failure";
    GenerateFn   m_generateFn;
    AssetFn      m_assetFn;

    std::vector<BuildIteration> m_iterations;
    IterationCallback           m_onIteration;
    IterationCallback           m_onApproved;
};

} // namespace Agents
