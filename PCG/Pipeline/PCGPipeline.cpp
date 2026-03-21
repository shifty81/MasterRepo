#include "PCG/Pipeline/PCGPipeline.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
namespace PCG {

// ─────────────────────────────────────────────────────────────────────────────
// Simple LCG random for deterministic seeding
// ─────────────────────────────────────────────────────────────────────────────

static uint64_t lcgNext(uint64_t& state)
{
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;
    return state;
}

static float lcgFloat(uint64_t& state, float lo, float hi)
{
    float t = (float)(lcgNext(state) & 0xFFFFFF) / (float)0x1000000;
    return lo + t * (hi - lo);
}

// ─────────────────────────────────────────────────────────────────────────────
// StarbaseGenerator
// ─────────────────────────────────────────────────────────────────────────────

StarbaseGenerator::StarbaseGenerator(StarbaseConfig cfg)
    : m_cfg(std::move(cfg))
{}

GeneratedModule StarbaseGenerator::MakeModule(const std::string& type,
                                               uint64_t& rng) const
{
    GeneratedModule m;
    m.id   = "mod_" + std::to_string(lcgNext(rng) & 0xFFFF);
    m.type = type;
    m.meshId = type + "_mesh";
    float spread = m_cfg.moduleSpacing;
    m.position = {
        lcgFloat(rng, -spread, spread),
        lcgFloat(rng, -spread * 0.5f, spread * 0.5f),
        lcgFloat(rng, -spread, spread)
    };
    m.rotation = {0.f, 0.f, 0.f, 1.f};
    return m;
}

GeneratedStructure StarbaseGenerator::Generate()
{
    uint64_t seed = (m_cfg.seed != 0) ? m_cfg.seed :
        (uint64_t)std::chrono::system_clock::now().time_since_epoch().count();
    return GenerateWithSeed(seed);
}

GeneratedStructure StarbaseGenerator::GenerateWithSeed(uint64_t seed)
{
    uint64_t rng = seed;

    GeneratedStructure s;
    s.id   = "structure_" + std::to_string(seed & 0xFFFF);
    s.type = m_cfg.type;
    s.seed = seed;

    // Required modules first
    for (const auto& req : m_cfg.requiredModules)
        s.modules.push_back(MakeModule(req, rng));

    // Fill up to random count
    int count = m_cfg.minModules + (int)(lcgNext(rng) % std::max(1,
                m_cfg.maxModules - m_cfg.minModules + 1));

    static const char* kTypes[] = {
        "core", "hangar", "reactor", "hab", "shield_gen",
        "turret", "storage", "lab", "medical", "command"
    };
    for (int i = (int)s.modules.size(); i < count; ++i) {
        const char* t = kTypes[lcgNext(rng) % 10];
        s.modules.push_back(MakeModule(t, rng));
    }

    if (m_cfg.withDocking)
        s.modules.push_back(MakeModule("docking_ring", rng));
    if (m_cfg.withDefences) {
        for (int i = 0; i < 4; ++i)
            s.modules.push_back(MakeModule("turret", rng));
    }

    s.valid = true;
    return s;
}

void StarbaseGenerator::SetConfig(const StarbaseConfig& cfg) { m_cfg = cfg; }
const StarbaseConfig& StarbaseGenerator::GetConfig() const { return m_cfg; }

// ─────────────────────────────────────────────────────────────────────────────
// ConstraintSolver
// ─────────────────────────────────────────────────────────────────────────────

void ConstraintSolver::AddConstraint(Constraint c)
{
    m_constraints.push_back(std::move(c));
}

void ConstraintSolver::RemoveConstraint(const std::string& name)
{
    m_constraints.erase(
        std::remove_if(m_constraints.begin(), m_constraints.end(),
                       [&](const Constraint& c){ return c.name == name; }),
        m_constraints.end());
}

SolverResult ConstraintSolver::Solve(const GeneratedStructure& structure) const
{
    SolverResult result;
    for (const auto& c : m_constraints) {
        if (!c.check(structure)) {
            ConstraintViolation v;
            v.constraintName = c.name;
            v.isSoft         = c.isSoft;
            v.message        = c.description;
            result.violations.push_back(v);
            if (!c.isSoft) result.passed = false;
        }
    }
    return result;
}

void   ConstraintSolver::Clear() { m_constraints.clear(); }
size_t ConstraintSolver::ConstraintCount() const { return m_constraints.size(); }

int SolverResult::HardFailCount() const
{
    int n = 0;
    for (const auto& v : violations) if (!v.isSoft) ++n;
    return n;
}

int SolverResult::SoftFailCount() const
{
    int n = 0;
    for (const auto& v : violations) if (v.isSoft) ++n;
    return n;
}

// ─────────────────────────────────────────────────────────────────────────────
// SandboxSim
// ─────────────────────────────────────────────────────────────────────────────

SandboxSim::SandboxSim(SandboxSimConfig cfg) : m_cfg(std::move(cfg)) {}

SandboxSimResult SandboxSim::Run(const GeneratedStructure& structure) const
{
    auto t0 = std::chrono::steady_clock::now();
    SandboxSimResult r;

    // Minimal checks
    if (m_cfg.checkNavMesh)
        r.navMeshValid = !structure.modules.empty();

    if (m_cfg.checkSpawnPts)
        r.spawnPointsValid = (structure.modules.size() >= 2);

    if (m_cfg.checkLOS)
        r.losValid = structure.valid;

    r.ticksSimulated = m_cfg.simTickCount;
    r.passed = r.navMeshValid && r.spawnPointsValid && r.losValid;

    auto t1 = std::chrono::steady_clock::now();
    r.simTimeSec = std::chrono::duration<float>(t1 - t0).count();
    return r;
}

void SandboxSim::SetConfig(const SandboxSimConfig& cfg) { m_cfg = cfg; }
const SandboxSimConfig& SandboxSim::GetConfig() const { return m_cfg; }

// ─────────────────────────────────────────────────────────────────────────────
// PCGPipeline
// ─────────────────────────────────────────────────────────────────────────────

PCGPipeline::PCGPipeline(PCGPipelineConfig cfg)
    : m_cfg(std::move(cfg))
    , m_generator(m_cfg.generator)
    , m_sandbox(m_cfg.sandbox)
{}

PCGPipeline::~PCGPipeline() {}

PCGPipelineResult PCGPipeline::Run() { return RunWithSeed(0); }

PCGPipelineResult PCGPipeline::RunWithSeed(uint64_t seed)
{
    PCGPipelineResult result;
    auto t0 = std::chrono::steady_clock::now();

    // Stage 1 — Generation
    if (m_cfg.runGeneration) {
        Notify("Generation", 0.1f);
        auto t = std::chrono::steady_clock::now();
        result.structure = (seed != 0)
                           ? m_generator.GenerateWithSeed(seed)
                           : m_generator.Generate();
        result.generationSec = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - t).count();

        if (!result.structure.valid) {
            result.passed = false;
            if (m_cfg.stopOnHardFail) goto done;
        }
    }

    // Stage 2 — Constraints
    if (m_cfg.runConstraints) {
        Notify("Constraints", 0.35f);
        auto t = std::chrono::steady_clock::now();
        result.constraintResult = m_solver.Solve(result.structure);
        result.constraintSec    = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - t).count();

        if (!result.constraintResult.passed) {
            result.passed = false;
            if (m_cfg.stopOnHardFail) goto done;
        }
    }

    // Stage 3 — Sandbox
    if (m_cfg.runSandbox) {
        Notify("Sandbox", 0.6f);
        auto t = std::chrono::steady_clock::now();
        result.sandboxResult = m_sandbox.Run(result.structure);
        result.sandboxSec    = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - t).count();

        if (!result.sandboxResult.passed) {
            result.passed = false;
            if (m_cfg.stopOnHardFail) goto done;
        }
    }

    // Stage 4 — Validation report
    if (m_cfg.runValidation) {
        Notify("Validation", 0.85f);
        result.validationReport = BuildReport(result);
    }

done:
    result.totalTimeSec = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - t0).count();

    Notify("Done", 1.0f);

    if (!m_cfg.outputPath.empty())
        SaveResult(result, m_cfg.outputPath);

    return result;
}

StarbaseGenerator& PCGPipeline::GetGenerator()      { return m_generator; }
ConstraintSolver&  PCGPipeline::GetConstraintSolver() { return m_solver; }
SandboxSim&        PCGPipeline::GetSandboxSim()      { return m_sandbox; }

void PCGPipeline::SetConfig(const PCGPipelineConfig& cfg) { m_cfg = cfg; }
const PCGPipelineConfig& PCGPipeline::GetConfig() const   { return m_cfg; }

void PCGPipeline::SetProgressCallback(ProgressFn fn)
{
    m_onProgress = std::move(fn);
}

void PCGPipeline::Notify(const std::string& stage, float pct)
{
    if (m_onProgress) m_onProgress(stage, pct);
}

std::string PCGPipeline::BuildReport(const PCGPipelineResult& result) const
{
    std::ostringstream oss;
    oss << "# PCG Pipeline Report\n\n"
        << "**Overall:** " << (result.passed ? "✅ PASS" : "❌ FAIL") << "\n"
        << "**Total time:** " << result.totalTimeSec << "s\n\n"
        << "## Structure\n"
        << "- ID: " << result.structure.id << "\n"
        << "- Modules: " << result.structure.modules.size() << "\n\n"
        << "## Constraints\n"
        << "- Hard failures: " << result.constraintResult.HardFailCount() << "\n"
        << "- Soft failures: " << result.constraintResult.SoftFailCount() << "\n\n"
        << "## Sandbox\n"
        << "- NavMesh: " << (result.sandboxResult.navMeshValid ? "OK" : "FAIL") << "\n"
        << "- Spawn pts: " << (result.sandboxResult.spawnPointsValid ? "OK" : "FAIL") << "\n"
        << "- LOS: " << (result.sandboxResult.losValid ? "OK" : "FAIL") << "\n";
    return oss.str();
}

bool PCGPipeline::SaveResult(const PCGPipelineResult& result,
                               const std::string& path) const
{
    try {
        fs::create_directories(fs::path(path).parent_path());
        std::ofstream f(path);
        if (!f) return false;
        f << BuildReport(result);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace PCG
