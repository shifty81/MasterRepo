#include "AI/MultiAgent/MultiAgentCoordinator.h"
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <sstream>

namespace AI {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

struct MultiAgentCoordinator::Impl {
    std::unordered_map<std::string, AgentFn>        agents;
    std::unordered_map<uint32_t, CoordinatedTask>   tasks;
    uint32_t                                        nextTaskId{1};
    uint32_t                                        completedCount{0};

    std::function<void(const CoordinatedTask&)>     onTaskComplete;
    std::function<void(uint32_t, const AgentResult&)> onAgentResult;

    AgentResult RunAgent(const std::string& name, const CoordinatedTask& task) {
        AgentResult res;
        res.agentName = name;
        uint64_t t0   = NowMs();
        auto it = agents.find(name);
        if (it == agents.end()) {
            res.errorMessage = "Agent not found: " + name;
            return res;
        }
        try {
            res.output      = it->second(task.prompt, task.context);
            res.succeeded   = true;
            res.confidenceScore = 0.7f; // real impl: agent returns confidence
        } catch (const std::exception& ex) {
            res.errorMessage = ex.what();
        }
        res.latencyMs = NowMs() - t0;
        if (onAgentResult) onAgentResult(task.id, res);
        return res;
    }

    std::string MergeResults(const std::vector<AgentResult>& results,
                             MergeStrategy strategy) {
        if (results.empty()) return {};
        switch (strategy) {
            case MergeStrategy::FirstSuccess:
                for (auto& r : results) if (r.succeeded) return r.output;
                return {};
            case MergeStrategy::Concatenate: {
                std::ostringstream ss;
                for (size_t i = 0; i < results.size(); ++i) {
                    if (i > 0) ss << "\n---\n";
                    ss << results[i].output;
                }
                return ss.str();
            }
            case MergeStrategy::HighestConfidence: {
                const AgentResult* best = nullptr;
                for (auto& r : results)
                    if (r.succeeded && (!best || r.confidenceScore > best->confidenceScore))
                        best = &r;
                return best ? best->output : std::string{};
            }
            case MergeStrategy::MajorityVote: {
                std::unordered_map<std::string,uint32_t> freq;
                for (auto& r : results) if (r.succeeded) ++freq[r.output];
                std::string best; uint32_t bestN = 0;
                for (auto& [k, v] : freq) if (v > bestN) { bestN = v; best = k; }
                return best;
            }
        }
        return {};
    }
};

MultiAgentCoordinator::MultiAgentCoordinator() : m_impl(new Impl()) {}
MultiAgentCoordinator::~MultiAgentCoordinator() { delete m_impl; }

void MultiAgentCoordinator::Init()     {}
void MultiAgentCoordinator::Shutdown() { m_impl->agents.clear(); m_impl->tasks.clear(); }

void MultiAgentCoordinator::RegisterAgent(const std::string& name, AgentFn fn) {
    m_impl->agents[name] = std::move(fn);
}

void MultiAgentCoordinator::UnregisterAgent(const std::string& name) {
    m_impl->agents.erase(name);
}

bool MultiAgentCoordinator::HasAgent(const std::string& name) const {
    return m_impl->agents.count(name) > 0;
}

std::vector<std::string> MultiAgentCoordinator::AgentNames() const {
    std::vector<std::string> out;
    for (auto& [k, _] : m_impl->agents) out.push_back(k);
    return out;
}

uint32_t MultiAgentCoordinator::CreateTask(const std::string& prompt,
                                           const std::string& context) {
    uint32_t id = m_impl->nextTaskId++;
    CoordinatedTask& t = m_impl->tasks[id];
    t.id      = id;
    t.prompt  = prompt;
    t.context = context;
    t.status  = AgentTaskStatus::Pending;
    t.startMs = NowMs();
    return id;
}

void MultiAgentCoordinator::CancelTask(uint32_t id) {
    auto it = m_impl->tasks.find(id);
    if (it != m_impl->tasks.end()) it->second.status = AgentTaskStatus::Cancelled;
}

CoordinatedTask MultiAgentCoordinator::GetTask(uint32_t id) const {
    auto it = m_impl->tasks.find(id);
    return it != m_impl->tasks.end() ? it->second : CoordinatedTask{};
}

CoordinatedTask MultiAgentCoordinator::FanOut(uint32_t taskId,
    const std::vector<std::string>& names, MergeStrategy strategy) {
    auto it = m_impl->tasks.find(taskId);
    if (it == m_impl->tasks.end()) return {};
    CoordinatedTask& task = it->second;
    task.status = AgentTaskStatus::Running;
    task.results.clear();
    for (auto& name : names)
        task.results.push_back(m_impl->RunAgent(name, task));
    task.mergedOutput = m_impl->MergeResults(task.results, strategy);
    task.status = AgentTaskStatus::Completed;
    task.endMs  = NowMs();
    ++m_impl->completedCount;
    if (m_impl->onTaskComplete) m_impl->onTaskComplete(task);
    return task;
}

CoordinatedTask MultiAgentCoordinator::Pipeline(uint32_t taskId,
    const std::vector<std::string>& names) {
    auto it = m_impl->tasks.find(taskId);
    if (it == m_impl->tasks.end()) return {};
    CoordinatedTask& task = it->second;
    task.status = AgentTaskStatus::Running;
    task.results.clear();
    std::string currentInput = task.prompt;
    for (auto& name : names) {
        CoordinatedTask step = task;
        step.prompt = currentInput;
        AgentResult res = m_impl->RunAgent(name, step);
        task.results.push_back(res);
        if (res.succeeded) currentInput = res.output;
    }
    task.mergedOutput = currentInput;
    task.status = AgentTaskStatus::Completed;
    task.endMs  = NowMs();
    ++m_impl->completedCount;
    if (m_impl->onTaskComplete) m_impl->onTaskComplete(task);
    return task;
}

AgentResult MultiAgentCoordinator::RunSingle(uint32_t taskId,
                                             const std::string& agentName) {
    auto it = m_impl->tasks.find(taskId);
    if (it == m_impl->tasks.end()) return {};
    return m_impl->RunAgent(agentName, it->second);
}

CoordinatedTask MultiAgentCoordinator::WaitForAll(uint32_t taskId,
                                                  uint32_t /*timeoutMs*/) {
    // Synchronous in this implementation (no async threads here)
    return GetTask(taskId);
}

uint32_t MultiAgentCoordinator::ActiveTaskCount()    const { return (uint32_t)m_impl->tasks.size() - m_impl->completedCount; }
uint32_t MultiAgentCoordinator::CompletedTaskCount() const { return m_impl->completedCount; }

void MultiAgentCoordinator::OnTaskComplete(std::function<void(const CoordinatedTask&)> cb) {
    m_impl->onTaskComplete = std::move(cb);
}

void MultiAgentCoordinator::OnAgentResult(
    std::function<void(uint32_t, const AgentResult&)> cb) {
    m_impl->onAgentResult = std::move(cb);
}

} // namespace AI
