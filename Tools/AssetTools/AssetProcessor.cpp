#include "Tools/AssetTools/AssetProcessor.h"
#include "Tools/Importer/AssetImporter.h"
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace Tools {

// ──────────────────────────────────────────────────────────────
// RLE codec (simple passthrough compression stub)
// ──────────────────────────────────────────────────────────────

std::vector<uint8_t> AssetProcessor::RunLengthEncode(const std::vector<uint8_t>& in) {
    std::vector<uint8_t> out;
    if (in.empty()) return out;
    size_t i = 0;
    while (i < in.size()) {
        uint8_t val = in[i];
        uint8_t run = 1;
        while (i + run < in.size() && in[i + run] == val && run < 255)
            ++run;
        out.push_back(run);
        out.push_back(val);
        i += run;
    }
    return out;
}

std::vector<uint8_t> AssetProcessor::RunLengthDecode(const std::vector<uint8_t>& in) {
    std::vector<uint8_t> out;
    for (size_t i = 0; i + 1 < in.size(); i += 2) {
        uint8_t run = in[i];
        uint8_t val = in[i+1];
        for (uint8_t k = 0; k < run; ++k) out.push_back(val);
    }
    return out;
}

// ──────────────────────────────────────────────────────────────
// Mesh optimisation (vertex deduplication on raw float stream)
// ──────────────────────────────────────────────────────────────

std::vector<uint8_t> AssetProcessor::OptimiseMeshPayload(const std::vector<uint8_t>& raw) {
    // Stub: return raw unchanged (full Assimp-based optimisation would go here)
    return raw;
}

// ──────────────────────────────────────────────────────────────
// Texture resize (nearest neighbour, RGBA8)
// ──────────────────────────────────────────────────────────────

std::vector<uint8_t> AssetProcessor::ResizeTexture(const std::vector<uint8_t>& raw,
                                                     uint32_t srcW, uint32_t srcH,
                                                     uint32_t dstW, uint32_t dstH) {
    if (srcW == 0 || srcH == 0 || dstW == 0 || dstH == 0) return {};
    if (raw.size() < (size_t)srcW * srcH * 4) return {};
    std::vector<uint8_t> out((size_t)dstW * dstH * 4);
    for (uint32_t y = 0; y < dstH; ++y) {
        for (uint32_t x = 0; x < dstW; ++x) {
            uint32_t sx = x * srcW / dstW;
            uint32_t sy = y * srcH / dstH;
            size_t srcIdx = ((size_t)sy * srcW + sx) * 4;
            size_t dstIdx = ((size_t)y  * dstW + x)  * 4;
            for (int c = 0; c < 4; ++c)
                out[dstIdx + c] = raw[srcIdx + c];
        }
    }
    return out;
}

// ──────────────────────────────────────────────────────────────
// Single-file processing
// ──────────────────────────────────────────────────────────────

ProcessResult AssetProcessor::ApplyOp(const std::string& atlasbPath,
                                       const ProcessOptions& opts) {
    ProcessResult res;

    AssetHeader hdr{};
    if (!AssetImporter::ReadHeader(atlasbPath, hdr)) {
        res.errorMessage = "Cannot read .atlasb header: " + atlasbPath;
        return res;
    }

    std::ifstream in(atlasbPath, std::ios::binary | std::ios::ate);
    if (!in) { res.errorMessage = "Cannot open: " + atlasbPath; return res; }
    auto totalSz = (size_t)in.tellg(); in.seekg(sizeof(AssetHeader));
    size_t payloadSz = totalSz - sizeof(AssetHeader);
    std::vector<uint8_t> payload(payloadSz);
    in.read(reinterpret_cast<char*>(payload.data()), (std::streamsize)payloadSz);
    in.close();

    res.inputBytes = (uint32_t)payloadSz;

    std::vector<uint8_t> out;
    switch (opts.op) {
    case ProcessOp::Compress:
        out = RunLengthEncode(payload); break;
    case ProcessOp::Decompress:
        out = RunLengthDecode(payload); break;
    case ProcessOp::OptimiseMesh:
        out = OptimiseMeshPayload(payload); break;
    case ProcessOp::Strip:
        out = payload; break; // no-op for now
    default:
        out = payload; break;
    }

    // Write back
    std::string outPath = atlasbPath; // overwrite in-place
    std::ofstream outp(outPath, std::ios::binary);
    if (!outp) { res.errorMessage = "Cannot write: " + outPath; return res; }
    hdr.size = (uint32_t)out.size();
    // recompute hash
    uint64_t hash = 14695981039346656037ULL;
    for (uint8_t b : out) { hash ^= b; hash *= 1099511628211ULL; }
    hdr.hash = hash;
    outp.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    outp.write(reinterpret_cast<const char*>(out.data()), (std::streamsize)out.size());

    res.success     = true;
    res.outputPath  = outPath;
    res.outputBytes = (uint32_t)out.size();
    return res;
}

ProcessResult AssetProcessor::Process(const std::string& atlasbPath,
                                       const ProcessOptions& opts) {
    return ApplyOp(atlasbPath, opts);
}

std::vector<ProcessResult> AssetProcessor::ProcessDirectory(const std::string& dirPath,
                                                             const ProcessOptions& opts,
                                                             bool recursive) {
    return ProcessDirectoryWithProgress(dirPath, opts, nullptr, recursive);
}

std::vector<ProcessResult> AssetProcessor::ProcessDirectoryWithProgress(
        const std::string& dirPath, const ProcessOptions& opts,
        ProgressFn progress, bool recursive) {
    std::vector<ProcessResult> results;
    if (!std::filesystem::exists(dirPath)) return results;

    std::vector<std::string> paths;
    auto collect = [&](const std::filesystem::directory_entry& e) {
        if (e.is_regular_file() && e.path().extension() == ".atlasb")
            paths.push_back(e.path().string());
    };
    if (recursive) {
        for (const auto& e : std::filesystem::recursive_directory_iterator(dirPath))
            collect(e);
    } else {
        for (const auto& e : std::filesystem::directory_iterator(dirPath))
            collect(e);
    }

    for (size_t i = 0; i < paths.size(); ++i) {
        if (progress) progress(i, paths.size(), paths[i]);
        results.push_back(ApplyOp(paths[i], opts));
    }
    if (progress) progress(paths.size(), paths.size(), "done");
    return results;
}

} // namespace Tools
