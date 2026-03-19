#include "LLMBackend.h"
#include "../core/Logger.h"
#include <algorithm>
#include <cstdlib>

namespace atlas::ai {

// ============================================================
// OfflineLLMBackend
// ============================================================

LLMResponse OfflineLLMBackend::Complete(const LLMRequest& request) {
    ++m_callCount;

    LLMResponse resp;
    resp.requestId = request.requestId != 0 ? request.requestId : m_nextRequestId++;
    resp.success = true;

    // Search for a canned response whose prefix matches the prompt.
    for (const auto& [prefix, text] : m_responses) {
        if (request.prompt.rfind(prefix, 0) == 0) {
            resp.text = text;
            resp.tokensUsed = static_cast<uint32_t>(text.size() / 4 + 1);
            resp.latencyMs = 0.0f;
            return resp;
        }
    }

    // Default offline response.
    resp.text = "[offline] No response registered for prompt: " + request.prompt;
    resp.tokensUsed = 8;
    resp.latencyMs = 0.0f;
    return resp;
}

bool OfflineLLMBackend::IsAvailable() const { return true; }

std::string OfflineLLMBackend::Name() const { return "OfflineStub"; }

uint8_t OfflineLLMBackend::Capabilities() const {
    return static_cast<uint8_t>(LLMCapability::TextGeneration);
}

void OfflineLLMBackend::RegisterResponse(const std::string& prefix, const std::string& response) {
    m_responses[prefix] = response;
}

size_t OfflineLLMBackend::ResponseCount() const {
    return m_responses.size();
}

void OfflineLLMBackend::ClearResponses() {
    m_responses.clear();
}

uint64_t OfflineLLMBackend::CallCount() const {
    return m_callCount;
}

// ============================================================
// LLMBackendRegistry
// ============================================================

void LLMBackendRegistry::SetBackend(std::shared_ptr<ILLMBackend> backend) {
    m_backend = std::move(backend);
}

ILLMBackend& LLMBackendRegistry::GetBackend() {
    if (m_backend) return *m_backend;
    return m_offlineStub;
}

const ILLMBackend& LLMBackendRegistry::GetBackend() const {
    if (m_backend) return *m_backend;
    return m_offlineStub;
}

bool LLMBackendRegistry::HasRealBackend() const {
    return m_backend != nullptr;
}

LLMResponse LLMBackendRegistry::Complete(const LLMRequest& request) {
    LLMRequest req = request;
    if (req.requestId == 0) req.requestId = m_nextRequestId++;

    LLMResponse resp = GetBackend().Complete(req);
    m_history.push_back(resp);
    return resp;
}

LLMResponse LLMBackendRegistry::Complete(const std::string& prompt) {
    LLMRequest req;
    req.prompt = prompt;
    req.requestId = m_nextRequestId++;
    return Complete(req);
}

const std::vector<LLMResponse>& LLMBackendRegistry::ResponseHistory() const {
    return m_history;
}

size_t LLMBackendRegistry::RequestCount() const {
    return m_history.size();
}

void LLMBackendRegistry::ClearHistory() {
    m_history.clear();
}

// ============================================================
// HttpLLMBackend
// ============================================================

HttpLLMBackend::HttpLLMBackend(atlas::asset::IHttpClient* httpClient,
                               const std::string& endpoint,
                               const std::string& model)
    : m_httpClient(httpClient)
    , m_endpoint(endpoint)
    , m_model(model) {}

LLMResponse HttpLLMBackend::Complete(const LLMRequest& request) {
    LLMResponse resp;
    resp.requestId = request.requestId != 0 ? request.requestId : m_nextRequestId++;

    if (!m_httpClient) {
        resp.success = false;
        resp.errorMessage = "No HTTP client configured";
        ++m_failureCount;
        return resp;
    }

    if (m_apiKey.empty()) {
        resp.success = false;
        resp.errorMessage = "No API key configured";
        ++m_failureCount;
        return resp;
    }

    std::string body = BuildRequestBody(request);

    std::vector<std::pair<std::string, std::string>> headers = {
        {"Authorization", "Bearer " + m_apiKey},
        {"Content-Type", "application/json"}
    };

    auto httpResp = m_httpClient->Post(m_endpoint, body, headers);

    if (httpResp.IsError()) {
        resp.success = false;
        resp.errorMessage = "HTTP error " + std::to_string(httpResp.statusCode) +
                            ": " + httpResp.errorMessage;
        ++m_failureCount;
        return resp;
    }

    resp = ParseResponse(httpResp.body, resp.requestId);
    if (resp.success) {
        ++m_successCount;
    } else {
        ++m_failureCount;
    }
    return resp;
}

bool HttpLLMBackend::IsAvailable() const {
    return m_httpClient != nullptr && !m_apiKey.empty();
}

std::string HttpLLMBackend::Name() const {
    return "HttpLLM(" + m_model + ")";
}

uint8_t HttpLLMBackend::Capabilities() const {
    return static_cast<uint8_t>(LLMCapability::TextGeneration) |
           static_cast<uint8_t>(LLMCapability::Streaming);
}

void HttpLLMBackend::SetApiKey(const std::string& apiKey) {
    m_apiKey = apiKey;
}

bool HttpLLMBackend::HasApiKey() const {
    return !m_apiKey.empty();
}

const std::string& HttpLLMBackend::GetEndpoint() const {
    return m_endpoint;
}

const std::string& HttpLLMBackend::GetModel() const {
    return m_model;
}

void HttpLLMBackend::SetTimeoutMs(uint32_t timeoutMs) {
    m_timeoutMs = timeoutMs;
}

uint32_t HttpLLMBackend::GetTimeoutMs() const {
    return m_timeoutMs;
}

uint64_t HttpLLMBackend::SuccessCount() const {
    return m_successCount;
}

uint64_t HttpLLMBackend::FailureCount() const {
    return m_failureCount;
}

std::string HttpLLMBackend::BuildRequestBody(const LLMRequest& request) const {
    // Minimal JSON construction without a JSON library.
    // Escapes double quotes and backslashes in user input.
    auto escape = [](const std::string& s) -> std::string {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            if (c == '"') out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else if (c == '\t') out += "\\t";
            else out += c;
        }
        return out;
    };

    std::string json = "{\"model\":\"" + escape(m_model) + "\",\"messages\":[";

    if (!request.systemPrompt.empty()) {
        json += "{\"role\":\"system\",\"content\":\"" + escape(request.systemPrompt) + "\"},";
    }

    json += "{\"role\":\"user\",\"content\":\"" + escape(request.prompt) + "\"}],";
    json += "\"temperature\":" + std::to_string(request.temperature) + ",";
    json += "\"max_tokens\":" + std::to_string(request.maxTokens) + "}";

    return json;
}

LLMResponse HttpLLMBackend::ParseResponse(const std::string& responseBody,
                                           uint64_t requestId) const {
    LLMResponse resp;
    resp.requestId = requestId;

    // Look for "content":"..." in the response JSON.
    const std::string key = "\"content\":\"";
    auto pos = responseBody.find(key);
    if (pos == std::string::npos) {
        resp.success = false;
        resp.errorMessage = "Could not find 'content' field in response";
        return resp;
    }

    pos += key.size();
    std::string content;
    bool escaped = false;
    for (size_t i = pos; i < responseBody.size(); ++i) {
        char c = responseBody[i];
        if (escaped) {
            if (c == 'n') content += '\n';
            else if (c == 't') content += '\t';
            else if (c == 'r') content += '\r';
            else content += c;
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '"') {
            break;
        } else {
            content += c;
        }
    }

    resp.text = content;
    resp.success = true;
    // Approximate token count: ~4 characters per token on average for English text.
    constexpr uint32_t kAvgCharsPerToken = 4;
    resp.tokensUsed = static_cast<uint32_t>(content.size() / kAvgCharsPerToken + 1);
    return resp;
}

// ============================================================
// LLMBackendFactory
// ============================================================

std::shared_ptr<HttpLLMBackend> LLMBackendFactory::CreateFromEnv(atlas::asset::IHttpClient* httpClient) {
    const char* endpoint = std::getenv("ATLAS_LLM_ENDPOINT");
    const char* model = std::getenv("ATLAS_LLM_MODEL");
    const char* apiKey = std::getenv("ATLAS_LLM_API_KEY");

    if (!endpoint || !model || !apiKey) {
        return nullptr;
    }

    uint32_t timeout = 30000;
    const char* timeoutStr = std::getenv("ATLAS_LLM_TIMEOUT");
    if (timeoutStr) {
        try { timeout = static_cast<uint32_t>(std::stoul(timeoutStr)); } catch (...) {}
    }

    return Create(httpClient, endpoint, model, apiKey, timeout);
}

std::shared_ptr<HttpLLMBackend> LLMBackendFactory::Create(
    atlas::asset::IHttpClient* httpClient,
    const std::string& endpoint,
    const std::string& model,
    const std::string& apiKey,
    uint32_t timeoutMs) {

    auto backend = std::make_shared<HttpLLMBackend>(httpClient, endpoint, model);
    backend->SetApiKey(apiKey);
    backend->SetTimeoutMs(timeoutMs);
    return backend;
}

bool LLMBackendFactory::HasEnvConfig() {
    return std::getenv("ATLAS_LLM_ENDPOINT") != nullptr &&
           std::getenv("ATLAS_LLM_MODEL") != nullptr &&
           std::getenv("ATLAS_LLM_API_KEY") != nullptr;
}

} // namespace atlas::ai
