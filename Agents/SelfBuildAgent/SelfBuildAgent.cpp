#include "Agents/SelfBuildAgent/SelfBuildAgent.h"
#include <sstream>
#include <fstream>
#include <cstdio>
#include <ctime>
#include <random>
#include <algorithm>
#include <filesystem>

namespace Agents {

namespace fs = std::filesystem;

// ── configuration ──────────────────────────────────────────────

void SelfBuildAgent::SetArchiveDir(const std::string& dir)   { m_archiveDir = dir; }
void SelfBuildAgent::SetMaxIterations(int max)               { m_maxIterations = max; }
void SelfBuildAgent::SetBuildCommand(const std::string& cmd) { m_buildCmd = cmd; }
void SelfBuildAgent::SetTestCommand (const std::string& cmd) { m_testCmd  = cmd; }
void SelfBuildAgent::SetGenerateCallback(GenerateFn fn)      { m_generateFn = std::move(fn); }
void SelfBuildAgent::SetAssetCallback(AssetFn fn)            { m_assetFn    = std::move(fn); }

void SelfBuildAgent::OnIterationComplete(IterationCallback cb) { m_onIteration = std::move(cb); }
void SelfBuildAgent::OnApproved(IterationCallback cb)          { m_onApproved  = std::move(cb); }

// ── run ────────────────────────────────────────────────────────

BuildIteration SelfBuildAgent::Run(const std::string& goal) {
    BuildIteration best;
    best.goal = goal;

    for (int i = 0; i < m_maxIterations; ++i) {
        BuildIteration iter = RunIteration(goal, i);
        m_iterations.push_back(iter);
        if (m_onIteration) m_onIteration(iter);

        if (iter.buildPassed && iter.testsPassed) {
            if (iter.scoreHeuristic >= best.scoreHeuristic) best = iter;
            break; // success on first passing iteration
        }
        if (iter.scoreHeuristic > best.scoreHeuristic) best = iter;
    }
    return best;
}

BuildIteration SelfBuildAgent::Step(const std::string& goal) {
    int attempt = static_cast<int>(m_iterations.size());
    BuildIteration iter = RunIteration(goal, attempt);
    m_iterations.push_back(iter);
    if (m_onIteration) m_onIteration(iter);
    return iter;
}

// ── review ─────────────────────────────────────────────────────

bool SelfBuildAgent::Approve(size_t index) {
    if (index >= m_iterations.size()) return false;
    BuildIteration& iter = m_iterations[index];
    // Write the generated code to the archive path for reference
    if (!iter.archivePath.empty()) {
        std::ofstream f(iter.archivePath + "/approved.txt");
        if (f.is_open()) f << "APPROVED\nGoal: " << iter.goal << "\n";
    }
    if (m_onApproved) m_onApproved(iter);
    return true;
}

void SelfBuildAgent::Reject(size_t index, const std::string& feedback) {
    if (index >= m_iterations.size()) return;
    BuildIteration& iter = m_iterations[index];
    if (!iter.archivePath.empty()) {
        std::ofstream f(iter.archivePath + "/rejected.txt");
        if (f.is_open()) f << "REJECTED\nFeedback: " << feedback << "\n";
    }
}

void SelfBuildAgent::ClearIterations() { m_iterations.clear(); }

// ── private helpers ────────────────────────────────────────────

BuildIteration SelfBuildAgent::RunIteration(const std::string& goal, int attempt) {
    BuildIteration iter;
    iter.id   = GenerateIterationId(attempt);
    iter.goal = goal;

    // 1. Generate code
    const BuildIteration* last = m_iterations.empty() ? nullptr : &m_iterations.back();
    if (m_generateFn)
        iter.generatedCode = m_generateFn(goal, last);
    else
        iter.generatedCode = "// TODO: connect AI code generator\n";

    // 2. Generate assets
    if (m_assetFn)
        iter.assetDescription = m_assetFn(goal);

    // 3. Archive
    ArchiveIteration(iter);

    // 4. Build & test
    DoBuild(iter);
    if (iter.buildPassed) DoTest(iter);

    // 5. Score: simple heuristic based on build/test status
    iter.scoreHeuristic = (iter.buildPassed ? 0.5 : 0.0)
                        + (iter.testsPassed ? 0.5 : 0.0);

    return iter;
}

bool SelfBuildAgent::DoBuild(BuildIteration& iter) {
    std::string cmd = m_buildCmd + " 2>&1";
    std::string output;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        char buf[256];
        while (fgets(buf, sizeof(buf), pipe)) output += buf;
        iter.buildPassed = (pclose(pipe) == 0);
    } else {
        iter.buildPassed = false;
        output = "Failed to run build command.";
    }
    iter.buildLog = std::move(output);
    return iter.buildPassed;
}

bool SelfBuildAgent::DoTest(BuildIteration& iter) {
    std::string cmd = m_testCmd + " 2>&1";
    std::string output;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        char buf[256];
        while (fgets(buf, sizeof(buf), pipe)) output += buf;
        iter.testsPassed = (pclose(pipe) == 0);
    } else {
        iter.testsPassed = false;
        output = "Failed to run test command.";
    }
    iter.testLog = std::move(output);
    return iter.testsPassed;
}

void SelfBuildAgent::ArchiveIteration(BuildIteration& iter) {
    std::string dir = m_archiveDir + "/" + iter.id;
    fs::create_directories(dir);
    iter.archivePath = dir;

    // Write generated code
    if (!iter.generatedCode.empty()) {
        std::ofstream f(dir + "/generated.cpp");
        if (f.is_open()) f << iter.generatedCode;
    }
    // Write metadata
    std::ofstream meta(dir + "/meta.txt");
    if (meta.is_open()) {
        meta << "id=" << iter.id << "\n";
        meta << "goal=" << iter.goal << "\n";
        meta << "assets=" << iter.assetDescription << "\n";
    }
}

std::string SelfBuildAgent::GenerateIterationId(int attempt) const {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", std::gmtime(&t));
    return std::string("iter_") + buf + "_" + std::to_string(attempt);
}

} // namespace Agents
