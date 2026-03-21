#pragma once
// MetaLearning — AI Meta-Learning Layer (Phase Blueprint — AI Intelligence)
//
// Cross-project knowledge transfer: the meta-learning layer aggregates
// successful patterns, code templates, and outcome histories from all agents
// (CodeAgent, PCGAgent, DebugAgent, etc.) and exposes a retrieval interface
// so future tasks can be seeded with prior art.
//
// Design goals
//   1. Offline only — no network calls; all storage is local JSON/text files.
//   2. Lightweight — regex + embedding similarity (no full LLM required).
//   3. Composable — works alongside AILib (MemoryStore, EmbeddingEngine).
//
// Typical flow:
//   • Agent completes a task  → Agent::RecordOutcome(...)
//   • Before next task       → MetaLearner::Query(goal) → top-k templates
//   • Templates injected into prompt context for the new task

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

namespace AI {

// ─────────────────────────────────────────────────────────────────────────────
// Data model
// ─────────────────────────────────────────────────────────────────────────────

enum class OutcomeQuality {
    Failed,
    Partial,
    Good,
    Excellent,
};

struct KnowledgeEntry {
    std::string     id;             // UUID-like key
    std::string     agentType;      // "CodeAgent", "PCGAgent", etc.
    std::string     task;           // human-readable task description
    std::string     context;        // project context snapshot
    std::string     solutionSnippet;// winning code / rule / prompt fragment
    OutcomeQuality  quality    = OutcomeQuality::Good;
    int             useCount   = 0;
    float           successRate= 1.0f; // 0..1
    uint64_t        timestamp  = 0;    // Unix seconds

    // Embedding vector (produced by EmbeddingEngine, stored for similarity)
    std::vector<float> embedding;
};

struct QueryResult {
    const KnowledgeEntry* entry = nullptr;
    float                 score = 0.f; // cosine similarity 0..1
};

// ─────────────────────────────────────────────────────────────────────────────
// MetaLearningConfig
// ─────────────────────────────────────────────────────────────────────────────

struct MetaLearningConfig {
    std::string storePath     = "TrainingData/MetaLearning";
    int         topK          = 5;        // how many results to return per query
    float       minScore      = 0.25f;    // discard results below this similarity
    int         maxEntries    = 10000;    // evict oldest beyond this
    bool        autoSave      = true;
    // Optional embedding function: text → float vector
    // Leave nullptr to fall back to keyword-overlap scoring.
    std::function<std::vector<float>(const std::string&)> embedFn;
};

// ─────────────────────────────────────────────────────────────────────────────
// MetaLearner
// ─────────────────────────────────────────────────────────────────────────────

class MetaLearner {
public:
    explicit MetaLearner(MetaLearningConfig cfg = {});
    ~MetaLearner();

    // ── Ingestion ────────────────────────────────────────────────────────────

    /// Record a completed task outcome.
    void RecordOutcome(const KnowledgeEntry& entry);

    /// Bulk-import entries (e.g. from archived training data).
    void Import(const std::vector<KnowledgeEntry>& entries);

    // ── Retrieval ────────────────────────────────────────────────────────────

    /// Retrieve top-k most relevant entries for a new task description.
    std::vector<QueryResult> Query(const std::string& task,
                                   int topK = -1) const;

    /// Filter by agent type before querying.
    std::vector<QueryResult> QueryForAgent(const std::string& agentType,
                                           const std::string& task,
                                           int topK = -1) const;

    // ── Management ───────────────────────────────────────────────────────────

    size_t EntryCount() const;
    void   Clear();

    /// Remove entries below a quality threshold.
    int Prune(OutcomeQuality minQuality);

    /// Re-compute embeddings for all entries (after changing embedFn).
    void RebuildEmbeddings();

    // ── Persistence ──────────────────────────────────────────────────────────

    bool Save() const;
    bool Load();

    // ── Stats ────────────────────────────────────────────────────────────────

    struct Stats {
        size_t totalEntries   = 0;
        size_t excellentCount = 0;
        size_t goodCount      = 0;
        size_t partialCount   = 0;
        size_t failedCount    = 0;
        float  avgSuccessRate = 0.f;
        std::unordered_map<std::string,int> byAgentType;
    };

    Stats GetStats() const;

private:
    MetaLearningConfig             m_cfg;
    std::vector<KnowledgeEntry>    m_entries;

    std::string GenerateId() const;
    float       ComputeScore(const KnowledgeEntry& entry,
                             const std::vector<float>& queryEmb,
                             const std::string& queryText) const;
    static float CosineSimilarity(const std::vector<float>& a,
                                   const std::vector<float>& b);
    static float KeywordOverlap(const std::string& a, const std::string& b);
};

} // namespace AI
