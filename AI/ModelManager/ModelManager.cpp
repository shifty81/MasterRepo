#include "AI/ModelManager/ModelManager.h"
#include <memory>

namespace AI {

ModelManager& ModelManager::Instance() {
    static ModelManager s_instance;
    return s_instance;
}

bool ModelManager::Init(const std::string& ollamaEndpoint, const std::string& model) {
    m_model = model;
    if (m_offlineMode) {
        m_registry.SetBackend(std::make_shared<OfflineLLMBackend>());
    }
    m_initialized = true;
    (void)ollamaEndpoint;
    return true;
}

bool ModelManager::IsReady() const {
    return m_initialized;
}

LLMResponse ModelManager::Complete(const std::string& prompt) {
    return m_registry.Complete(prompt);
}

LLMResponse ModelManager::Complete(const LLMRequest& request) {
    return m_registry.Complete(request);
}

void ModelManager::SetModel(const std::string& model) { m_model = model; }
const std::string& ModelManager::GetModel() const { return m_model; }

void ModelManager::SetOfflineMode(bool offline) {
    m_offlineMode = offline;
    if (offline) {
        m_registry.SetBackend(std::make_shared<OfflineLLMBackend>());
    }
}

bool ModelManager::IsOfflineMode() const { return m_offlineMode; }

LLMBackendRegistry& ModelManager::GetRegistry() { return m_registry; }
ILLMBackend&        ModelManager::GetBackend()   { return m_registry.GetBackend(); }

} // namespace AI
