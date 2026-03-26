#include "Builder/Packaging/PackagingPipeline.h"
#include <atomic>
#include <chrono>
#include <fstream>
#include <sstream>

namespace Builder {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

struct PackagingPipeline::Impl {
    PackagingConfig  config;
    PackagingResult  lastResult;
    float            progress{0.f};
    std::string      currentStep;
    std::atomic<bool> running{false};

    std::function<void(float)>               onProgress;
    std::function<void(const std::string&)>  onStep;
    std::function<void(const PackagingResult&)> onComplete;
    std::function<void(const std::string&)>  onError;

    void Step(const std::string& name, float pct) {
        currentStep = name;
        progress    = pct;
        if (onStep)     onStep(name);
        if (onProgress) onProgress(pct);
    }
};

PackagingPipeline::PackagingPipeline() : m_impl(new Impl()) {}
PackagingPipeline::~PackagingPipeline() { delete m_impl; }

void PackagingPipeline::Init(const PackagingConfig& cfg) { m_impl->config = cfg; }
void PackagingPipeline::Shutdown() {}

PackagingResult PackagingPipeline::Package() {
    m_impl->running = true;
    m_impl->progress = 0.f;
    PackagingResult res;
    uint64_t t0 = NowMs();

    // Step 1: Collect assets
    m_impl->Step("Collecting assets", 0.1f);
    res.assetCount = 0; // would scan content dirs

    // Step 2: Strip debug / editor data
    if (m_impl->config.stripDebug || m_impl->config.stripEditor)
        m_impl->Step("Stripping debug data", 0.3f);

    // Step 3: Compress
    m_impl->Step("Compressing assets", 0.5f);
    std::string codec = "none";
    if (m_impl->config.compression == CompressionType::LZ4)  codec = "lz4";
    if (m_impl->config.compression == CompressionType::Zstd) codec = "zstd";

    // Step 4: Write .pak
    m_impl->Step("Writing bundle", 0.7f);
    std::string pakPath = m_impl->config.outputDir + "/" +
                          m_impl->config.gameName + "_" +
                          m_impl->config.version + ".pak";
    {
        std::ofstream f(pakPath, std::ios::binary);
        if (f.is_open()) {
            // Minimal PAK header
            std::string header = "PAKV1 codec=" + codec + " game=" +
                                  m_impl->config.gameName + "\n";
            f.write(header.data(), (std::streamsize)header.size());
            res.packedBytes = (uint64_t)header.size();
        }
    }
    res.pakPath = pakPath;

    // Step 5: Write manifest
    m_impl->Step("Writing manifest", 0.9f);
    std::string manifestPath = m_impl->config.outputDir + "/manifest.json";
    {
        std::ofstream f(manifestPath);
        if (f.is_open()) {
            f << "{\"game\":\"" << m_impl->config.gameName << "\","
              << "\"version\":\"" << m_impl->config.version << "\","
              << "\"platform\":\"" << m_impl->config.platform << "\","
              << "\"codec\":\"" << codec << "\","
              << "\"assets\":" << res.assetCount << "}\n";
        }
    }
    res.manifestPath = manifestPath;

    m_impl->Step("Done", 1.0f);
    res.succeeded   = true;
    res.durationMs  = NowMs() - t0;
    m_impl->lastResult = res;
    m_impl->running    = false;

    if (m_impl->onComplete) m_impl->onComplete(res);
    return res;
}

bool PackagingPipeline::IsRunning()    const { return m_impl->running; }
void PackagingPipeline::Cancel()             { m_impl->running = false; }
float PackagingPipeline::Progress()    const { return m_impl->progress; }
std::string PackagingPipeline::CurrentStep() const { return m_impl->currentStep; }
PackagingResult PackagingPipeline::LastResult() const { return m_impl->lastResult; }
PackagingConfig PackagingPipeline::GetConfig()  const { return m_impl->config; }
void PackagingPipeline::SetConfig(const PackagingConfig& c) { m_impl->config = c; }

void PackagingPipeline::OnProgress(std::function<void(float)> cb)      { m_impl->onProgress = std::move(cb); }
void PackagingPipeline::OnStepStart(std::function<void(const std::string&)> cb) { m_impl->onStep = std::move(cb); }
void PackagingPipeline::OnComplete(std::function<void(const PackagingResult&)> cb) { m_impl->onComplete = std::move(cb); }
void PackagingPipeline::OnError(std::function<void(const std::string&)> cb) { m_impl->onError = std::move(cb); }

} // namespace Builder
