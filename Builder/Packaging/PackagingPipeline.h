#pragma once
/**
 * @file PackagingPipeline.h
 * @brief Final game packaging pipeline — bundles assets, compresses archives,
 *        generates a platform manifest, and produces a distributable build.
 *
 * Steps:
 *   1. Collect all referenced assets from the content tree
 *   2. Strip debug symbols and editor-only data
 *   3. Compress assets using the registered compressor (LZ4 / Zstd)
 *   4. Write a binary asset bundle (.pak) and a JSON manifest
 *   5. Copy engine binaries and config to the output directory
 *
 * Typical usage:
 * @code
 *   PackagingPipeline pkg;
 *   PackagingConfig cfg;
 *   cfg.outputDir     = "Dist/Win64";
 *   cfg.platform      = "Win64";
 *   cfg.stripDebug    = true;
 *   cfg.compression   = CompressionType::Zstd;
 *   pkg.Init(cfg);
 *   pkg.Package();
 *   pkg.OnProgress([](float p){ printf("%.0f%%\n", p*100); });
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Builder {

// ── Compression codec ─────────────────────────────────────────────────────────

enum class CompressionType : uint8_t { None = 0, LZ4 = 1, Zstd = 2 };

// ── Packaging configuration ───────────────────────────────────────────────────

struct PackagingConfig {
    std::string     outputDir;
    std::string     platform{"Win64"};   ///< Win64 | Linux | macOS
    std::string     gameName{"NovaForge"};
    std::string     version{"1.0.0"};
    bool            stripDebug{true};
    bool            stripEditor{true};
    CompressionType compression{CompressionType::Zstd};
    std::vector<std::string> extraAssetDirs;   ///< additional dirs to bundle
    std::vector<std::string> excludePatterns;  ///< glob patterns to skip
};

// ── Packaging result ──────────────────────────────────────────────────────────

struct PackagingResult {
    bool        succeeded{false};
    std::string pakPath;          ///< path to generated .pak file
    std::string manifestPath;     ///< path to generated manifest JSON
    uint64_t    totalBytes{0};    ///< uncompressed asset bytes
    uint64_t    packedBytes{0};   ///< compressed bundle size
    uint32_t    assetCount{0};
    std::string errorMessage;
    uint64_t    durationMs{0};
};

// ── PackagingPipeline ─────────────────────────────────────────────────────────

class PackagingPipeline {
public:
    PackagingPipeline();
    ~PackagingPipeline();

    void Init(const PackagingConfig& config = {});
    void Shutdown();

    // ── Build ─────────────────────────────────────────────────────────────────

    PackagingResult Package();
    bool            IsRunning() const;
    void            Cancel();

    // ── Query ─────────────────────────────────────────────────────────────────

    float           Progress()      const; ///< 0–1
    std::string     CurrentStep()   const;
    PackagingResult LastResult()    const;

    // ── Configuration ─────────────────────────────────────────────────────────

    PackagingConfig GetConfig() const;
    void            SetConfig(const PackagingConfig& config);

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnProgress(std::function<void(float progress)> cb);
    void OnStepStart(std::function<void(const std::string& step)> cb);
    void OnComplete(std::function<void(const PackagingResult&)> cb);
    void OnError(std::function<void(const std::string& message)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Builder
