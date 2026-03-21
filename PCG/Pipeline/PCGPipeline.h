#pragma once
// PCGPipeline — Master PCG pipeline controller (Blueprint: pcg_pipeline)
//
// Coordinates all PCG sub-systems into a sequential, dependency-respecting
// pipeline:
//   • Starbase / structure generator (uses PCGRule + WorldNode + MeshNode)
//   • Constraint solver — validates generated content against design rules
//   • Sandbox simulation — runs a quick in-engine sim to check playability
//   • Validation — PCGValidator integrity check + anomaly detection
//
// Each stage can be run independently or as part of the full pipeline.
// Results are accumulated in a PCGPipelineResult.

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

namespace PCG {

// ─────────────────────────────────────────────────────────────────────────────
// Starbase / Structure generator config
// ─────────────────────────────────────────────────────────────────────────────

enum class StructureType {
    Starbase,
    SpaceStation,
    DungeonLevel,
    TerrainChunk,
    Settlement,
    Custom,
};

struct StarbaseConfig {
    StructureType type      = StructureType::Starbase;
    uint64_t      seed      = 0;   // 0 = random
    int           minModules = 5;
    int           maxModules = 30;
    float         moduleSpacing = 20.f;  // metres
    bool          withDocking  = true;
    bool          withDefences = true;
    std::string   theme;             // "industrial", "organic", "alien"
    std::vector<std::string> requiredModules;
};

struct GeneratedModule {
    std::string  id;
    std::string  type;
    std::array<float,3> position;
    std::array<float,4> rotation;  // quaternion
    std::string  meshId;
};

struct GeneratedStructure {
    std::string                   id;
    StructureType                 type;
    uint64_t                      seed     = 0;
    std::vector<GeneratedModule>  modules;
    std::string                   errorMsg;
    bool                          valid    = false;
};

class StarbaseGenerator {
public:
    explicit StarbaseGenerator(StarbaseConfig cfg = {});

    GeneratedStructure Generate();
    GeneratedStructure GenerateWithSeed(uint64_t seed);

    void SetConfig(const StarbaseConfig& cfg);
    const StarbaseConfig& GetConfig() const;

private:
    StarbaseConfig m_cfg;
    GeneratedModule MakeModule(const std::string& type, uint64_t& rng) const;
};

// ─────────────────────────────────────────────────────────────────────────────
// Constraint solver
// ─────────────────────────────────────────────────────────────────────────────

struct Constraint {
    std::string name;
    std::string description;
    std::function<bool(const GeneratedStructure&)> check;
    bool        isSoft = false;  // soft = warn, not fail
};

struct ConstraintViolation {
    std::string constraintName;
    bool        isSoft   = false;
    std::string message;
};

struct SolverResult {
    bool                            passed = true;
    std::vector<ConstraintViolation> violations;

    int HardFailCount() const;
    int SoftFailCount() const;
};

class ConstraintSolver {
public:
    void AddConstraint(Constraint c);
    void RemoveConstraint(const std::string& name);

    SolverResult Solve(const GeneratedStructure& structure) const;
    void         Clear();
    size_t       ConstraintCount() const;

private:
    std::vector<Constraint> m_constraints;
};

// ─────────────────────────────────────────────────────────────────────────────
// Sandbox simulation (playability check)
// ─────────────────────────────────────────────────────────────────────────────

struct SandboxSimConfig {
    int   simTickCount  = 100;    // fixed-tick steps to run
    float tickDelta     = 0.033f; // ~30fps
    bool  checkNavMesh  = true;
    bool  checkSpawnPts = true;
    bool  checkLOS      = true;   // line-of-sight between critical nodes
};

struct SandboxSimResult {
    bool   passed            = true;
    bool   navMeshValid      = true;
    bool   spawnPointsValid  = true;
    bool   losValid          = true;
    int    ticksSimulated    = 0;
    float  simTimeSec        = 0.f;
    std::string errorMsg;
};

class SandboxSim {
public:
    explicit SandboxSim(SandboxSimConfig cfg = {});

    SandboxSimResult Run(const GeneratedStructure& structure) const;

    void SetConfig(const SandboxSimConfig& cfg);
    const SandboxSimConfig& GetConfig() const;

private:
    SandboxSimConfig m_cfg;
};

// ─────────────────────────────────────────────────────────────────────────────
// PCGPipelineResult
// ─────────────────────────────────────────────────────────────────────────────

struct PCGPipelineResult {
    bool                passed          = true;
    GeneratedStructure  structure;
    SolverResult        constraintResult;
    SandboxSimResult    sandboxResult;
    std::string         validationReport;
    float               totalTimeSec    = 0.f;

    // Stage durations
    float  generationSec  = 0.f;
    float  constraintSec  = 0.f;
    float  sandboxSec     = 0.f;
    float  validationSec  = 0.f;
};

// ─────────────────────────────────────────────────────────────────────────────
// PCGPipelineConfig
// ─────────────────────────────────────────────────────────────────────────────

struct PCGPipelineConfig {
    StarbaseConfig    generator;
    SandboxSimConfig  sandbox;
    bool runGeneration  = true;
    bool runConstraints = true;
    bool runSandbox     = true;
    bool runValidation  = true;
    bool stopOnHardFail = true;   // skip remaining stages on hard failure
    std::string outputPath;       // write result JSON here if non-empty
};

// ─────────────────────────────────────────────────────────────────────────────
// PCGPipeline
// ─────────────────────────────────────────────────────────────────────────────

class PCGPipeline {
public:
    explicit PCGPipeline(PCGPipelineConfig cfg = {});
    ~PCGPipeline();

    // ── Run ──────────────────────────────────────────────────────────────────

    PCGPipelineResult Run();
    PCGPipelineResult RunWithSeed(uint64_t seed);

    // ── Stage access ─────────────────────────────────────────────────────────

    StarbaseGenerator& GetGenerator();
    ConstraintSolver&  GetConstraintSolver();
    SandboxSim&        GetSandboxSim();

    // ── Configuration ────────────────────────────────────────────────────────

    void SetConfig(const PCGPipelineConfig& cfg);
    const PCGPipelineConfig& GetConfig() const;

    // ── Progress callback ────────────────────────────────────────────────────

    using ProgressFn = std::function<void(const std::string& stage, float pct)>;
    void SetProgressCallback(ProgressFn fn);

    // ── Result I/O ───────────────────────────────────────────────────────────

    bool SaveResult(const PCGPipelineResult& result,
                    const std::string& path) const;
    std::string BuildReport(const PCGPipelineResult& result) const;

private:
    PCGPipelineConfig  m_cfg;
    StarbaseGenerator  m_generator;
    ConstraintSolver   m_solver;
    SandboxSim         m_sandbox;
    ProgressFn         m_onProgress;

    void Notify(const std::string& stage, float pct);
};

} // namespace PCG
