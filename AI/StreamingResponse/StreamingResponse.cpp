#include "AI/StreamingResponse/StreamingResponse.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace AI {

// ── helpers ───────────────────────────────────────────────────────────────────

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

// ── Impl ─────────────────────────────────────────────────────────────────────

struct StreamingResponse::Impl {
    std::map<uint64_t, StreamSession>                                  sessions;
    uint64_t                                                           nextId{1};
    std::function<void(uint64_t, const StreamChunk&)>                  onChunk;
    std::function<void(const StreamSession&)>                          onComplete;
    std::function<void(const StreamSession&)>                          onError;
    StreamingResponseStats                                             stats{};
};

// ── StreamingResponse ─────────────────────────────────────────────────────────

StreamingResponse::StreamingResponse() : m_impl(new Impl{}) {}
StreamingResponse::~StreamingResponse() { delete m_impl; }

uint64_t StreamingResponse::Begin(const std::string& prompt,
                                   const std::string& model) {
    uint64_t id = m_impl->nextId++;
    StreamSession s{};
    s.id      = id;
    s.prompt  = prompt;
    s.model   = model;
    s.state   = StreamState::Streaming;
    s.startMs = NowMs();
    m_impl->sessions[id] = s;
    ++m_impl->stats.totalSessions;
    return id;
}

void StreamingResponse::FeedChunk(uint64_t sessionId,
                                   const std::string& text,
                                   bool isFinal) {
    auto it = m_impl->sessions.find(sessionId);
    if (it == m_impl->sessions.end()) return;
    StreamSession& s = it->second;
    if (s.state != StreamState::Streaming) return;

    StreamChunk chunk{};
    chunk.text        = text;
    chunk.seqNo       = s.chunkCount;
    chunk.timestampMs = NowMs();
    chunk.isFinal     = isFinal;

    s.fullText   += text;
    ++s.chunkCount;
    ++m_impl->stats.totalChunks;

    // Rough token approximation: count whitespace-delimited words
    size_t words = 0;
    bool inWord  = false;
    for (char c : text) {
        bool sp = (c == ' ' || c == '\t' || c == '\n' || c == '\r');
        if (!sp && !inWord) { ++words; inWord = true; }
        else if (sp)        { inWord = false; }
    }
    m_impl->stats.totalTokensApprox += words > 0 ? words : 1;

    if (m_impl->onChunk) m_impl->onChunk(sessionId, chunk);

    if (isFinal) {
        s.state = StreamState::Completed;
        s.endMs = NowMs();
        ++m_impl->stats.completedSessions;
        if (m_impl->onComplete) m_impl->onComplete(s);
    }
}

void StreamingResponse::SetError(uint64_t sessionId, const std::string& message) {
    auto it = m_impl->sessions.find(sessionId);
    if (it == m_impl->sessions.end()) return;
    StreamSession& s = it->second;
    s.state        = StreamState::Error;
    s.errorMessage = message;
    s.endMs        = NowMs();
    ++m_impl->stats.errorSessions;
    if (m_impl->onError) m_impl->onError(s);
}

void StreamingResponse::Cancel(uint64_t sessionId) {
    auto it = m_impl->sessions.find(sessionId);
    if (it == m_impl->sessions.end()) return;
    StreamSession& s = it->second;
    if (s.state == StreamState::Streaming) {
        s.state = StreamState::Cancelled;
        s.endMs = NowMs();
        ++m_impl->stats.cancelledSessions;
    }
}

StreamSession StreamingResponse::GetSession(uint64_t sessionId) const {
    auto it = m_impl->sessions.find(sessionId);
    return it != m_impl->sessions.end() ? it->second : StreamSession{};
}

StreamState StreamingResponse::GetState(uint64_t sessionId) const {
    auto it = m_impl->sessions.find(sessionId);
    return it != m_impl->sessions.end() ? it->second.state : StreamState::Idle;
}

std::string StreamingResponse::GetFullText(uint64_t sessionId) const {
    auto it = m_impl->sessions.find(sessionId);
    return it != m_impl->sessions.end() ? it->second.fullText : "";
}

std::vector<StreamSession> StreamingResponse::AllSessions() const {
    std::vector<StreamSession> result;
    result.reserve(m_impl->sessions.size());
    for (const auto& [id, s] : m_impl->sessions) result.push_back(s);
    return result;
}

void StreamingResponse::DiscardSession(uint64_t sessionId) {
    m_impl->sessions.erase(sessionId);
}

void StreamingResponse::ClearHistory() { m_impl->sessions.clear(); }

void StreamingResponse::OnChunk(
    std::function<void(uint64_t, const StreamChunk&)> callback) {
    m_impl->onChunk = std::move(callback);
}

void StreamingResponse::OnComplete(
    std::function<void(const StreamSession&)> callback) {
    m_impl->onComplete = std::move(callback);
}

void StreamingResponse::OnError(
    std::function<void(const StreamSession&)> callback) {
    m_impl->onError = std::move(callback);
}

StreamingResponseStats StreamingResponse::Stats() const { return m_impl->stats; }

} // namespace AI
