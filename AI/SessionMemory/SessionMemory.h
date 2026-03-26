#pragma once
/**
 * @file SessionMemory.h
 * @brief Persistent, multi-session memory store for AI agents.
 *
 * Provides key-value and vector-embedded memory that survives between editor
 * sessions.  Backed by a JSON file on disk; embeddings are stored as float
 * vectors for similarity search.
 *
 * Typical usage:
 * @code
 *   SessionMemory mem;
 *   mem.Init("WorkspaceState/ai_memory.json");
 *   mem.Store("last_project", "NovaForge");
 *   mem.StoreEmbedding("ship_design_v3", {0.1f, 0.8f, ...});
 *   std::string project = mem.Recall("last_project");
 *   auto similar = mem.FindSimilar({0.1f, 0.8f, ...}, 5);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace AI {

// ── A stored memory entry ─────────────────────────────────────────────────────

struct MemoryEntry {
    std::string          key;
    std::string          value;         ///< plain text value
    std::vector<float>   embedding;     ///< optional dense vector
    uint64_t             timestampMs{0};
    uint32_t             accessCount{0};
    std::string          sessionTag;    ///< which session created this
};

// ── Similarity search result ──────────────────────────────────────────────────

struct MemorySimilarityResult {
    std::string key;
    float       score{0.0f}; ///< cosine similarity 0-1
    std::string value;
};

// ── SessionMemory ─────────────────────────────────────────────────────────────

class SessionMemory {
public:
    SessionMemory();
    ~SessionMemory();

    /// Initialise and load existing memory from disk.
    void Init(const std::string& persistPath = "WorkspaceState/ai_memory.json");
    void Shutdown();

    // ── Store / recall ────────────────────────────────────────────────────────

    void        Store(const std::string& key, const std::string& value,
                      const std::string& sessionTag = "");

    void        StoreEmbedding(const std::string& key,
                               const std::vector<float>& embedding,
                               const std::string& value = "",
                               const std::string& sessionTag = "");

    std::string Recall(const std::string& key) const;
    bool        Has(const std::string& key)    const;
    void        Forget(const std::string& key);

    // ── Similarity search ─────────────────────────────────────────────────────

    std::vector<MemorySimilarityResult>
    FindSimilar(const std::vector<float>& query, uint32_t topK = 5) const;

    // ── Persistence ───────────────────────────────────────────────────────────

    void Save() const;
    void Load();
    void ClearDisk();

    // ── Query ─────────────────────────────────────────────────────────────────

    std::vector<MemoryEntry> AllEntries() const;
    std::vector<MemoryEntry> EntriesForSession(const std::string& tag) const;
    uint32_t                 Count() const;

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnStore(std::function<void(const MemoryEntry&)> cb);
    void OnForget(std::function<void(const std::string& key)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace AI
