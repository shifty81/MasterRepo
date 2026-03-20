#pragma once
// AI Model Manager — LLM Backend Interface
// Provides ILLMBackend abstraction over different LLM providers.

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <cstdint>

namespace AI {

enum class LLMCapability : uint8_t {
    TextGeneration  = 1 << 0,
    Streaming       = 1 << 1,
    Embeddings      = 1 << 2,
    FunctionCalling = 1 << 3,
};

struct LLMRequest {
    uint64_t    requestId    = 0;
    std::string prompt;
    std::string systemPrompt;
    float       temperature  = 0.7f;
    uint32_t    maxTokens    = 2048;
};

struct LLMResponse {
    uint64_t    requestId    = 0;
    std::string text;
    bool        success      = false;
    std::string errorMessage;
    uint32_t    tokensUsed   = 0;
    float       latencyMs    = 0.0f;
};

// Minimal HTTP client interface (no external dependency)
struct HttpResponse {
    int         statusCode   = 0;
    std::string body;
    std::string errorMessage;
    bool IsError() const { return statusCode < 200 || statusCode >= 300; }
};

class IHttpClient {
public:
    virtual ~IHttpClient() = default;
    virtual HttpResponse Post(const std::string& url,
                              const std::string& body,
                              const std::vector<std::pair<std::string,std::string>>& headers) = 0;
};

class ILLMBackend {
public:
    virtual ~ILLMBackend() = default;
    virtual LLMResponse Complete(const LLMRequest& request) = 0;
    virtual bool        IsAvailable() const = 0;
    virtual std::string Name() const = 0;
    virtual uint8_t     Capabilities() const = 0;
};

class OfflineLLMBackend : public ILLMBackend {
public:
    LLMResponse Complete(const LLMRequest& request) override;
    bool        IsAvailable() const override;
    std::string Name() const override;
    uint8_t     Capabilities() const override;

    void   RegisterResponse(const std::string& prefix, const std::string& response);
    size_t ResponseCount() const;
    void   ClearResponses();
    uint64_t CallCount() const;

private:
    std::map<std::string, std::string> m_responses;
    uint64_t m_callCount     = 0;
    uint64_t m_nextRequestId = 1;
};

class HttpLLMBackend : public ILLMBackend {
public:
    HttpLLMBackend(IHttpClient* httpClient,
                   const std::string& endpoint,
                   const std::string& model);

    LLMResponse Complete(const LLMRequest& request) override;
    bool        IsAvailable() const override;
    std::string Name() const override;
    uint8_t     Capabilities() const override;

    void              SetApiKey(const std::string& apiKey);
    bool              HasApiKey() const;
    const std::string& GetEndpoint() const;
    const std::string& GetModel() const;
    void              SetTimeoutMs(uint32_t timeoutMs);
    uint32_t          GetTimeoutMs() const;

    uint64_t SuccessCount() const;
    uint64_t FailureCount() const;

private:
    std::string  BuildRequestBody(const LLMRequest& request) const;
    LLMResponse  ParseResponse(const std::string& responseBody, uint64_t requestId) const;

    IHttpClient* m_httpClient   = nullptr;
    std::string  m_endpoint;
    std::string  m_model;
    std::string  m_apiKey;
    uint32_t     m_timeoutMs    = 30000;
    uint64_t     m_successCount = 0;
    uint64_t     m_failureCount = 0;
    uint64_t     m_nextRequestId = 1;
};

class LLMBackendRegistry {
public:
    void               SetBackend(std::shared_ptr<ILLMBackend> backend);
    ILLMBackend&       GetBackend();
    const ILLMBackend& GetBackend() const;
    bool               HasRealBackend() const;

    LLMResponse Complete(const LLMRequest& request);
    LLMResponse Complete(const std::string& prompt);

    const std::vector<LLMResponse>& ResponseHistory() const;
    size_t                          RequestCount() const;
    void                            ClearHistory();

private:
    std::shared_ptr<ILLMBackend> m_backend;
    OfflineLLMBackend            m_offlineStub;
    std::vector<LLMResponse>     m_history;
    uint64_t                     m_nextRequestId = 1;
};

class LLMBackendFactory {
public:
    static std::shared_ptr<HttpLLMBackend> CreateFromEnv(IHttpClient* httpClient);
    static std::shared_ptr<HttpLLMBackend> Create(IHttpClient* httpClient,
                                                   const std::string& endpoint,
                                                   const std::string& model,
                                                   const std::string& apiKey,
                                                   uint32_t timeoutMs = 30000);
    static bool HasEnvConfig();
};

} // namespace AI
