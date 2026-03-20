#pragma once
#include "Agents/CodeAgent/CodeAgent.h"
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <cstdint>

namespace AI {

enum class TaskPriority : uint8_t { Low = 0, Normal = 1, High = 2, Critical = 3 };

enum class TaskStatus : uint8_t { Pending, Running, Completed, Cancelled, Failed };

struct AgentTask {
    uint64_t           id         = 0;
    std::string        agentName;
    Agents::AIRequest  request;
    TaskPriority       priority   = TaskPriority::Normal;
    TaskStatus         status     = TaskStatus::Pending;
    Agents::AIResponse result;
};

class AgentScheduler {
public:
    void    RegisterAgent(const std::string& name, Agents::CodeAgent* agent);
    uint64_t SubmitTask(const std::string& agentName, const Agents::AIRequest& request,
                        TaskPriority priority = TaskPriority::Normal);
    void    CancelTask(uint64_t id);
    void    Tick();

    AgentTask* GetTask(uint64_t id);
    size_t     GetPendingCount() const;
    size_t     GetCompletedCount() const;
    std::vector<AgentTask> DrainCompleted();
    void       SetMaxConcurrent(size_t n);

private:
    std::map<std::string, Agents::CodeAgent*> m_agents;
    std::vector<AgentTask>                    m_tasks;
    uint64_t                                  m_nextTaskId     = 1;
    size_t                                    m_maxConcurrent  = 4;
    size_t                                    m_completedCount = 0;
};

} // namespace AI
