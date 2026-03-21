#include "Tools/AssetPipeline/AssetPipeline.h"
#include <chrono>
#include <algorithm>
#include <optional>
#include <filesystem>

namespace Tools {

struct AssetPipeline::Impl {
    std::vector<PipelineJob>    queue;
    std::vector<JobResult>      history;
    uint64_t                    nextId{1};
    PipelineProgressFn          progressFn;
    PipelineCompleteFn          completeFn;
};

AssetPipeline::AssetPipeline() : m_impl(new Impl()) {}
AssetPipeline::~AssetPipeline() { delete m_impl; }

uint64_t AssetPipeline::Enqueue(PipelineJob job) {
    job.id = m_impl->nextId++;
    m_impl->queue.push_back(std::move(job));
    return m_impl->queue.back().id;
}

void AssetPipeline::Cancel(uint64_t id) {
    auto& q = m_impl->queue;
    q.erase(std::remove_if(q.begin(), q.end(),
                [id](const PipelineJob& j){ return j.id == id; }), q.end());
}

void AssetPipeline::CancelAll() { m_impl->queue.clear(); }

size_t AssetPipeline::PendingCount() const  { return m_impl->queue.size(); }
size_t AssetPipeline::TotalEnqueued() const { return m_impl->nextId - 1; }

static double nowMs() {
    using namespace std::chrono;
    return static_cast<double>(
        duration_cast<microseconds>(
            high_resolution_clock::now().time_since_epoch()).count()) / 1000.0;
}

JobResult AssetPipeline::executeJob(PipelineJob& job) {
    JobResult res;
    res.id = job.id;
    double t0 = nowMs();

    auto reportProgress = [&](JobState st, float pct) {
        res.state = st;
        if (m_impl->progressFn) m_impl->progressFn(job.id, st, pct);
    };

    // ── Import ──────────────────────────────────────────────
    reportProgress(JobState::Importing, 0.0f);
    if (job.assetType == AssetType::Unknown) {
        job.assetType = AssetImporter::InferType(job.sourcePath);
    }
    ImportOptions opts = job.importOptions;
    if (opts.outputDirectory.empty()) {
        // Use temp directory alongside the source
        namespace fs = std::filesystem;
        std::error_code ec;
        opts.outputDirectory = fs::temp_directory_path(ec).string();
    }
    ImportResult importRes = AssetImporter::Import(job.sourcePath, opts);
    if (!importRes.success) {
        res.state        = JobState::Failed;
        res.errorMessage = "Import failed: " + importRes.errorMessage;
        res.elapsedMs    = nowMs() - t0;
        return res;
    }
    reportProgress(JobState::Importing, 1.0f);

    // ── Process steps ────────────────────────────────────────
    if (!job.processSteps.empty()) {
        reportProgress(JobState::Processing, 0.0f);
        std::string current = importRes.outputPath;
        for (size_t i = 0; i < job.processSteps.size(); ++i) {
            auto& step = job.processSteps[i];
            ProcessResult pr = AssetProcessor::Process(current, step);
            if (!pr.success) {
                res.state        = JobState::Failed;
                res.errorMessage = "Process step failed: " + pr.errorMessage;
                res.elapsedMs    = nowMs() - t0;
                return res;
            }
            current = pr.outputPath;
            float pct = static_cast<float>(i + 1) / static_cast<float>(job.processSteps.size());
            reportProgress(JobState::Processing, pct);
        }
        importRes.outputPath = current;
    }

    // ── Export (copy processed asset to final output path) ───
    reportProgress(JobState::Exporting, 0.0f);
    {
        namespace fs = std::filesystem;
        std::error_code ec;
        bool same = (importRes.outputPath == job.outputPath);
        if (!same && !importRes.outputPath.empty()) {
            // Ensure parent directory exists
            fs::path outP(job.outputPath);
            if (outP.has_parent_path())
                fs::create_directories(outP.parent_path(), ec);
            fs::copy_file(importRes.outputPath, job.outputPath,
                          fs::copy_options::overwrite_existing, ec);
            if (ec) {
                res.state        = JobState::Failed;
                res.errorMessage = "Export failed: " + ec.message();
                res.elapsedMs    = nowMs() - t0;
                return res;
            }
        }
    }

    res.state      = JobState::Completed;
    res.outputPath = job.outputPath;
    res.success    = true;
    res.elapsedMs  = nowMs() - t0;
    return res;
}

std::vector<JobResult> AssetPipeline::RunAll() {
    std::vector<JobResult> results;
    while (!m_impl->queue.empty()) {
        auto r = RunNext();
        if (r) results.push_back(*r);
    }
    return results;
}

std::optional<JobResult> AssetPipeline::RunNext() {
    if (m_impl->queue.empty()) return std::nullopt;
    PipelineJob job = std::move(m_impl->queue.front());
    m_impl->queue.erase(m_impl->queue.begin());
    JobResult res = executeJob(job);
    m_impl->history.push_back(res);
    if (m_impl->completeFn) m_impl->completeFn(res);
    return res;
}

JobResult AssetPipeline::Process(const std::string& source,
                                   const std::string& output,
                                   AssetType type,
                                   const std::vector<ProcessOptions>& steps)
{
    PipelineJob job;
    job.sourcePath    = source;
    job.outputPath    = output;
    job.assetType     = type;
    job.processSteps  = steps;
    job.id            = m_impl->nextId++;
    return executeJob(job);
}

void AssetPipeline::OnProgress(PipelineProgressFn fn) {
    m_impl->progressFn = std::move(fn);
}
void AssetPipeline::OnComplete(PipelineCompleteFn fn) {
    m_impl->completeFn = std::move(fn);
}

const std::vector<JobResult>& AssetPipeline::History() const {
    return m_impl->history;
}
void AssetPipeline::ClearHistory() { m_impl->history.clear(); }

} // namespace Tools
