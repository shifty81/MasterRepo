#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace AI {

// ──────────────────────────────────────────────────────────────
// Sample kinds
// ──────────────────────────────────────────────────────────────

enum class SampleKind { CodeSnippet, ErrorFix, AIPrompt, AssetDescription, Diff };

// ──────────────────────────────────────────────────────────────
// Raw training sample
// ──────────────────────────────────────────────────────────────

struct RawSample {
    SampleKind  kind       = SampleKind::CodeSnippet;
    std::string input;        // prompt / error / code
    std::string output;       // expected completion / fix / description
    std::string sourceFile;   // where collected from
    std::string tag;
    uint64_t    timestamp  = 0;
};

// ──────────────────────────────────────────────────────────────
// Aggregate stats
// ──────────────────────────────────────────────────────────────

struct CollectorStats {
    size_t total      = 0;
    size_t code       = 0;
    size_t errorFixes = 0;
    size_t prompts    = 0;
    size_t assets     = 0;
    size_t diffs      = 0;
};

// ──────────────────────────────────────────────────────────────
// TrainingCollector — gathers, filters and exports training data
// ──────────────────────────────────────────────────────────────

class TrainingCollector {
public:
    explicit TrainingCollector(const std::string& dataRoot);  // e.g. "TrainingData/"

    // Collect
    void AddSample(RawSample s);
    void CollectFromDirectory(const std::string& dir, SampleKind kind);

    // Filter & clean
    void FilterMinLength(size_t minInputLen, size_t minOutputLen);
    void DeduplicateByInput();

    // Export (JSONL — one JSON object per line)
    bool ExportCodeSamples(const std::string& outPath) const;
    bool ExportErrorFixes (const std::string& outPath) const;
    bool ExportAll        (const std::string& outPath) const;

    // Stats
    CollectorStats GetStats() const;
    size_t Count() const;
    void   Clear();

    const std::vector<RawSample>& Samples() const { return m_samples; }

private:
    bool writeSamples(const std::vector<RawSample>& samples,
                      const std::string& path) const;

    std::string            m_root;
    std::vector<RawSample> m_samples;
};

} // namespace AI
