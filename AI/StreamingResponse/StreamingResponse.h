#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace AI {

// ── Data types ────────────────────────────────────────────────────────────────

enum class StreamState : uint8_t {
    Idle       = 0,
    Streaming  = 1,
    Completed  = 2,
    Error      = 3,
    Cancelled  = 4,
};

struct StreamChunk {
    std::string text;          // incremental token(s) received
    uint64_t    seqNo{0};      // chunk sequence number (0-based)
    uint64_t    timestampMs{0};
    bool        isFinal{false};
};

struct StreamSession {
    uint64_t    id{0};
    std::string prompt;
    std::string model;
    StreamState state{StreamState::Idle};
    std::string fullText;      // accumulated response so far
    uint64_t    startMs{0};
    uint64_t    endMs{0};
    uint64_t    chunkCount{0};
    std::string errorMessage;
};

struct StreamingResponseStats {
    uint64_t totalSessions{0};
    uint64_t completedSessions{0};
    uint64_t cancelledSessions{0};
    uint64_t errorSessions{0};
    uint64_t totalChunks{0};
    uint64_t totalTokensApprox{0};
};

// ── StreamingResponse ─────────────────────────────────────────────────────────

class StreamingResponse {
public:
    StreamingResponse();
    ~StreamingResponse();

    // ── Session lifecycle ─────────────────────────────────────────────────────

    // Begin a new streaming session with the given prompt and optional model.
    // Returns a session id.
    uint64_t Begin(const std::string& prompt, const std::string& model = "");

    // Feed a chunk of text into the active session (called by transport layer
    // as data arrives from the local LLM or HTTP stream).
    // isFinal = true marks the end of the stream.
    void FeedChunk(uint64_t sessionId, const std::string& text, bool isFinal = false);

    // Mark a session as errored.
    void SetError(uint64_t sessionId, const std::string& message);

    // Cancel an in-progress session.
    void Cancel(uint64_t sessionId);

    // ── Query ─────────────────────────────────────────────────────────────────

    StreamSession       GetSession(uint64_t sessionId) const;
    StreamState         GetState(uint64_t sessionId)   const;
    std::string         GetFullText(uint64_t sessionId) const;

    // All completed/active sessions.
    std::vector<StreamSession> AllSessions() const;

    // Remove a session from history.
    void DiscardSession(uint64_t sessionId);

    // Remove all sessions.
    void ClearHistory();

    // ── Callbacks ─────────────────────────────────────────────────────────────

    // Fired for every chunk received (any session).
    void OnChunk(std::function<void(uint64_t sessionId, const StreamChunk&)> callback);

    // Fired when a session transitions to Completed.
    void OnComplete(std::function<void(const StreamSession&)> callback);

    // Fired when a session transitions to Error.
    void OnError(std::function<void(const StreamSession&)> callback);

    // ── Stats ─────────────────────────────────────────────────────────────────

    StreamingResponseStats Stats() const;

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace AI
