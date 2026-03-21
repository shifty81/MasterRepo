#pragma once
/**
 * @file AssetPipeline.h
 * @brief Unified asset import → process → export pipeline.
 *
 * AssetPipeline orchestrates the full asset lifecycle:
 *   1. Import  — load a raw source file via AssetImporter
 *   2. Process — apply operations (compress, mipmap, optimise) via AssetProcessor
 *   3. Export  — write the final .atlasb binary
 *
 * Jobs are queued and run sequentially (or via JobSystem in a future phase).
 * Progress and completion are reported via callbacks.
 *
 * This wraps the existing Tools::AssetImporter and Tools::AssetProcessor
 * classes into a single high-level facade.
 */

#include "Tools/Importer/AssetImporter.h"
#include "Tools/AssetTools/AssetProcessor.h"
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <optional>

namespace Tools {

// ── Job definition ────────────────────────────────────────────────────────
struct PipelineJob {
    uint64_t    id{0};
    std::string sourcePath;          ///< Raw source (obj/png/wav/etc.)
    std::string outputPath;          ///< Final .atlasb destination
    AssetType   assetType{AssetType::Unknown};

    /// Optional process steps (applied in order after import)
    std::vector<ProcessOptions> processSteps;

    /// Optional import overrides
    ImportOptions importOptions;
};

enum class JobState : uint8_t {
    Pending,
    Importing,
    Processing,
    Exporting,
    Completed,
    Failed,
};

struct JobResult {
    uint64_t    id{0};
    JobState    state{JobState::Pending};
    std::string outputPath;
    std::string errorMessage;
    double      elapsedMs{0.0};
    bool        success{false};
};

// ── Callbacks ─────────────────────────────────────────────────────────────
using PipelineProgressFn = std::function<void(uint64_t jobId,
                                               JobState state,
                                               float progress)>; // 0..1
using PipelineCompleteFn = std::function<void(const JobResult&)>;

// ── Pipeline ──────────────────────────────────────────────────────────────
class AssetPipeline {
public:
    AssetPipeline();
    ~AssetPipeline();

    // ── job queue ─────────────────────────────────────────────
    uint64_t Enqueue(PipelineJob job);
    void     Cancel(uint64_t id);
    void     CancelAll();
    size_t   PendingCount() const;
    size_t   TotalEnqueued() const;

    // ── execution ─────────────────────────────────────────────
    /// Process all pending jobs synchronously.
    std::vector<JobResult> RunAll();

    /// Process the next pending job; returns result or nullopt if queue empty.
    std::optional<JobResult> RunNext();

    // ── convenience: single-shot import+process+export ────────
    JobResult Process(const std::string& source,
                      const std::string& output,
                      AssetType type = AssetType::Unknown,
                      const std::vector<ProcessOptions>& steps = {});

    // ── callbacks ─────────────────────────────────────────────
    void OnProgress(PipelineProgressFn fn);
    void OnComplete(PipelineCompleteFn fn);

    // ── stats ─────────────────────────────────────────────────
    const std::vector<JobResult>& History() const;
    void ClearHistory();

private:
    struct Impl;
    Impl* m_impl{nullptr};

    JobResult executeJob(PipelineJob& job);
};

} // namespace Tools
