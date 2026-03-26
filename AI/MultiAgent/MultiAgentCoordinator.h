#pragma once
/**
 * @file MultiAgentCoordinator.h
 * @brief Orchestrates multiple AI agents running concurrently with task
 *        distribution, result merging, and consensus voting.
 *
 * Supports:
 *   - Fan-out: broadcast a task to N agents and collect all responses
 *   - Pipeline: chain agents (output of one → input of next)
 *   - Voting: agents vote on best answer; majority or confidence-weighted
 *   - Priority queue: high-urgency tasks preempt normal queue
 *
 * Typical usage:
 * @code
 *   MultiAgentCoordinator coord;
 *   coord.Init();
 *   coord.RegisterAgent("refactor",  refactorAgentFn);
 *   coord.RegisterAgent("review",    reviewAgentFn);
 *   coord.RegisterAgent("docstring", docAgentFn);
 *
 *   auto task = coord.CreateTask("Improve this function", code);
 *   coord.FanOut(task, {"refactor","review","docstring"});
 *   auto merged = coord.WaitForAll(task, 30000);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace AI {

// ── Agent function type ───────────────────────────────────────────────────────

using AgentFn = std::function<std::string(const std::string& prompt,
                                          const std::string& context)>;

// ── Task status ───────────────────────────────────────────────────────────────

enum class AgentTaskStatus : uint8_t {
    Pending = 0, Running, Completed, Failed, Cancelled
};

// ── Individual agent result ───────────────────────────────────────────────────

struct AgentResult {
    std::string agentName;
    std::string output;
    float       confidenceScore{0.5f};  ///< 0–1
    uint64_t    latencyMs{0};
    bool        succeeded{false};
    std::string errorMessage;
};

// ── Coordinated task ──────────────────────────────────────────────────────────

struct CoordinatedTask {
    uint32_t              id{0};
    std::string           prompt;
    std::string           context;
    AgentTaskStatus       status{AgentTaskStatus::Pending};
    std::vector<AgentResult> results;
    std::string           mergedOutput;
    uint64_t              startMs{0};
    uint64_t              endMs{0};
};

// ── Merge / consensus strategies ─────────────────────────────────────────────

enum class MergeStrategy : uint8_t {
    FirstSuccess    = 0, ///< return first successful result
    Concatenate     = 1, ///< join all outputs with separator
    HighestConfidence = 2, ///< pick output with highest confidence score
    MajorityVote    = 3, ///< most common answer wins
};

// ── MultiAgentCoordinator ─────────────────────────────────────────────────────

class MultiAgentCoordinator {
public:
    MultiAgentCoordinator();
    ~MultiAgentCoordinator();

    void Init();
    void Shutdown();

    // ── Agent registry ────────────────────────────────────────────────────────

    void RegisterAgent(const std::string& name, AgentFn fn);
    void UnregisterAgent(const std::string& name);
    bool HasAgent(const std::string& name) const;
    std::vector<std::string> AgentNames() const;

    // ── Task creation ─────────────────────────────────────────────────────────

    uint32_t CreateTask(const std::string& prompt,
                        const std::string& context = "");
    void     CancelTask(uint32_t taskId);
    CoordinatedTask GetTask(uint32_t taskId) const;

    // ── Execution patterns ────────────────────────────────────────────────────

    /// Run the same task on multiple agents simultaneously and merge results.
    CoordinatedTask FanOut(uint32_t taskId,
                           const std::vector<std::string>& agentNames,
                           MergeStrategy strategy = MergeStrategy::HighestConfidence);

    /// Chain agents in sequence: output of agent[i] feeds into agent[i+1].
    CoordinatedTask Pipeline(uint32_t taskId,
                             const std::vector<std::string>& agentNames);

    /// Run on a single named agent.
    AgentResult RunSingle(uint32_t taskId, const std::string& agentName);

    // ── Blocking wait ─────────────────────────────────────────────────────────

    CoordinatedTask WaitForAll(uint32_t taskId, uint32_t timeoutMs = 30000);

    // ── Stats ─────────────────────────────────────────────────────────────────

    uint32_t ActiveTaskCount()    const;
    uint32_t CompletedTaskCount() const;

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnTaskComplete(std::function<void(const CoordinatedTask&)> cb);
    void OnAgentResult(std::function<void(uint32_t taskId,
                                          const AgentResult&)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace AI
