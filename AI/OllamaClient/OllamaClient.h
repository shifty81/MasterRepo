#pragma once
/**
 * @file OllamaClient.h
 * @brief AI-LV-01 — Thin Ollama HTTP client for local LLM inference.
 *
 * Communicates with a locally-running Ollama server (default: localhost:11434).
 * Uses POSIX sockets (TCP) directly to avoid a libcurl/libhttp dependency.
 *
 * Usage:
 *   OllamaClient client("localhost", 11434);
 *   if (client.Ping()) { ... }
 *   auto resp = client.Generate("codellama", "Fix this build error: ...");
 *
 * AI-LV-02: Generate() produces code; caller pastes it at editor cursor.
 * AI-LV-03: FixBuildError() wraps Generate with an error-fix system prompt.
 * AI-LV-04: PCGPromptToScene() translates a user description to PCG params.
 */
#include <cstdint>
#include <functional>
#include <string>

namespace AI {

struct OllamaResponse {
    bool        success  = false;
    std::string text;       ///< Generated / replied text
    std::string error;      ///< Error message if !success
    int         httpStatus  = 0;
};

class OllamaClient {
public:
    OllamaClient(const std::string& host = "localhost", uint16_t port = 11434);

    // ── AI-LV-01: connectivity ────────────────────────────────────────
    /// Returns true if the Ollama server is reachable and responds to /api/tags.
    bool Ping();

    /// Returns the list of locally-pulled model names (comma-separated).
    std::string ListModels();

    // ── AI-LV-02/03/04: inference ─────────────────────────────────────
    /// Blocking single-turn generate call to Ollama /api/generate endpoint.
    OllamaResponse Generate(const std::string& model,
                            const std::string& prompt,
                            const std::string& system = "");

    /// AI-LV-02: Generate code for `task` using the code-generation model.
    OllamaResponse GenerateCode(const std::string& task,
                                const std::string& context = "");

    /// AI-LV-03: Ask the LLM to fix the supplied build error.
    OllamaResponse FixBuildError(const std::string& errorText,
                                 const std::string& sourceSnippet = "");

    /// AI-LV-04: Translate a natural-language scene description to PCG JSON params.
    OllamaResponse PCGPromptToScene(const std::string& description);

    // ── Configuration ─────────────────────────────────────────────────
    void SetModel(const std::string& model)   { m_model = model; }
    void SetTimeout(int timeoutSeconds)       { m_timeout = timeoutSeconds; }
    const std::string& Host() const           { return m_host; }
    uint16_t           Port() const           { return m_port; }

    // ── Streaming callback ────────────────────────────────────────────
    /// When set, each token chunk is forwarded as it arrives (newline-JSON).
    using StreamCallback = std::function<void(const std::string& chunk, bool done)>;
    void SetStreamCallback(StreamCallback cb) { m_streamCb = std::move(cb); }

private:
    /// Send a raw HTTP POST to path with body; return raw response.
    std::string HttpPost(const std::string& path, const std::string& body);
    /// Send a raw HTTP GET; return response body.
    std::string HttpGet(const std::string& path);
    /// Extract the "response" field from a /api/generate JSON object.
    static std::string ExtractField(const std::string& json, const std::string& key);

    std::string    m_host;
    uint16_t       m_port;
    std::string    m_model   = "codellama";
    int            m_timeout = 30;   // seconds
    StreamCallback m_streamCb;
};

} // namespace AI
