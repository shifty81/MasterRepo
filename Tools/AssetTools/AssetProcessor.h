#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

namespace Tools {

// ──────────────────────────────────────────────────────────────
// Asset processing operations
// ──────────────────────────────────────────────────────────────

enum class ProcessOp {
    Compress,           // compress payload (LZ-style stub)
    Decompress,
    GenerateMipmaps,    // texture mipmap chain
    OptimiseMesh,       // remove duplicate verts, fix winding
    ResizeTexture,      // nearest-neighbour resize
    ConvertFormat,      // e.g. RGB→RGBA, PCM→float
    Strip               // remove metadata / debug data
};

struct ProcessOptions {
    ProcessOp   op               = ProcessOp::Compress;
    uint32_t    targetWidth      = 0;    // ResizeTexture
    uint32_t    targetHeight     = 0;    // ResizeTexture
    std::string targetFormat;            // ConvertFormat target name
    bool        overwrite        = true;
};

struct ProcessResult {
    bool        success      = false;
    std::string outputPath;
    std::string errorMessage;
    uint32_t    inputBytes   = 0;
    uint32_t    outputBytes  = 0;

    float CompressionRatio() const {
        return (inputBytes > 0) ? (float)outputBytes / (float)inputBytes : 0.f;
    }
};

// ──────────────────────────────────────────────────────────────
// AssetProcessor — batch conversions and optimizations
// ──────────────────────────────────────────────────────────────

class AssetProcessor {
public:
    // Process a single .atlasb file
    static ProcessResult Process(const std::string& atlasbPath,
                                  const ProcessOptions& opts);

    // Batch process all .atlasb files in a directory
    static std::vector<ProcessResult> ProcessDirectory(const std::string& dirPath,
                                                        const ProcessOptions& opts,
                                                        bool recursive = false);

    // Progress callback for batch operations
    using ProgressFn = std::function<void(size_t done, size_t total,
                                          const std::string& current)>;
    static std::vector<ProcessResult> ProcessDirectoryWithProgress(
        const std::string& dirPath, const ProcessOptions& opts,
        ProgressFn progress, bool recursive = false);

    // Standalone utilities (operate on raw byte buffers)
    static std::vector<uint8_t> RunLengthEncode(const std::vector<uint8_t>& in);
    static std::vector<uint8_t> RunLengthDecode(const std::vector<uint8_t>& in);

    static std::vector<uint8_t> OptimiseMeshPayload(const std::vector<uint8_t>& raw);
    static std::vector<uint8_t> ResizeTexture(const std::vector<uint8_t>& raw,
                                               uint32_t srcW, uint32_t srcH,
                                               uint32_t dstW, uint32_t dstH);

private:
    static ProcessResult ApplyOp(const std::string& atlasbPath,
                                  const ProcessOptions& opts);
};

} // namespace Tools
