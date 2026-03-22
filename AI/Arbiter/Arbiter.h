#pragma once
#include <string>
#include <vector>

namespace AI {
namespace Arbiter {

class ArbiterOrchestrator {
public:
    // Conversational interface entry point
    std::string Converse(const std::string& userInput);
    // Plan and execute a workflow
    bool ExecuteWorkflow(const std::string& workflowName, const std::vector<std::string>& args);
    // Query the knowledge base
    std::vector<std::string> QueryKnowledgeBase(const std::string& query);
};

} // namespace Arbiter
} // namespace AI
