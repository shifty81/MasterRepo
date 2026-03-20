#pragma once
#include "AI/ModelManager/LLMBackend.h"
#include <string>
#include <memory>

namespace AI {

class ModelManager {
public:
    static ModelManager& Instance();

    bool Init(const std::string& ollamaEndpoint, const std::string& model = "llama3");
    bool IsReady() const;

    LLMResponse Complete(const std::string& prompt);
    LLMResponse Complete(const LLMRequest& request);

    void SetModel(const std::string& model);
    const std::string& GetModel() const;

    void SetOfflineMode(bool offline);
    bool IsOfflineMode() const;

    LLMBackendRegistry& GetRegistry();
    ILLMBackend&        GetBackend();

private:
    ModelManager() = default;

    LLMBackendRegistry m_registry;
    std::string        m_model;
    bool               m_offlineMode = false;
    bool               m_initialized = false;
};

} // namespace AI
