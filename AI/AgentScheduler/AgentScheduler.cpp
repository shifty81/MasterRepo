#include "AI/AgentScheduler/AgentScheduler.h"
#include <algorithm>

namespace AI {

void AgentScheduler::RegisterAgent(const std::string& name, Agents::CodeAgent* agent) {
    m_agents[name] = agent;
}

uint64_t AgentScheduler::SubmitTask(const std::string& agentName,
                                     const Agents::AIRequest& request,
                                     TaskPriority priority) {
    AgentTask task;
    task.id = m_nextTaskId++;
    task.agentName = agentName;
    task.request = request;
    task.priority = priority;
    task.status = TaskStatus::Pending;
    m_tasks.push_back(task);
    return task.id;
}

void AgentScheduler::CancelTask(uint64_t id) {
    for (auto& t : m_tasks) {
        if (t.id == id && t.status == TaskStatus::Pending) {
            t.status = TaskStatus::Cancelled;
        }
    }
}

void AgentScheduler::Tick() {
    size_t running = 0;
    for (auto& t : m_tasks)
        if (t.status == TaskStatus::Running) ++running;

    std::vector<AgentTask*> pending;
    for (auto& t : m_tasks)
        if (t.status == TaskStatus::Pending) pending.push_back(&t);
    std::sort(pending.begin(), pending.end(),
        [](const AgentTask* a, const AgentTask* b){
            return static_cast<uint8_t>(a->priority) > static_cast<uint8_t>(b->priority);
        });

    for (auto* taskPtr : pending) {
        if (running >= m_maxConcurrent) break;
        auto it = m_agents.find(taskPtr->agentName);
        if (it == m_agents.end()) {
            taskPtr->status = TaskStatus::Failed;
            taskPtr->result.errorMessage = "Agent not found: " + taskPtr->agentName;
            ++m_completedCount;
            continue;
        }
        taskPtr->status = TaskStatus::Running;
        taskPtr->result = it->second->ProcessRequest(taskPtr->request);
        taskPtr->status = taskPtr->result.success ? TaskStatus::Completed : TaskStatus::Failed;
        ++m_completedCount;
        ++running;
    }
}

AgentTask* AgentScheduler::GetTask(uint64_t id) {
    for (auto& t : m_tasks)
        if (t.id == id) return &t;
    return nullptr;
}

size_t AgentScheduler::GetPendingCount() const {
    size_t n = 0;
    for (const auto& t : m_tasks)
        if (t.status == TaskStatus::Pending) ++n;
    return n;
}

size_t AgentScheduler::GetCompletedCount() const { return m_completedCount; }

std::vector<AgentTask> AgentScheduler::DrainCompleted() {
    std::vector<AgentTask> done;
    std::vector<AgentTask> remaining;
    for (auto& t : m_tasks) {
        if (t.status == TaskStatus::Completed || t.status == TaskStatus::Failed ||
            t.status == TaskStatus::Cancelled) {
            done.push_back(t);
        } else {
            remaining.push_back(t);
        }
    }
    m_tasks = std::move(remaining);
    return done;
}

void AgentScheduler::SetMaxConcurrent(size_t n) { m_maxConcurrent = n; }

} // namespace AI
