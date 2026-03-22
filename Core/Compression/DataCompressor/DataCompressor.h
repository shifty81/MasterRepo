#pragma once
/**
 * @file DataCompressor.h
 * @brief Data compression and decompression supporting RLE and LZ-style algorithms.
 *
 * DataCompressor provides in-memory compression/decompression without external
 * library dependencies:
 *
 *   CompressionAlgo: None, RLE, LZ (custom LZ77-style), LZH (LZ + Huffman-like).
 *
 *   CompressedBlob: compressed data bytes, originalSize, algo, and a validity flag.
 *
 *   DataCompressor (static API — no instance needed):
 *     - Compress(data, size, algo) → CompressedBlob
 *     - Decompress(blob) → vector<uint8_t>
 *     - CompressString(s, algo) → CompressedBlob
 *     - DecompressString(blob) → std::string
 *     - EstimateRatio(data, size) → float  (heuristic: 1.0 = no compression)
 *     - AlgoName(algo) → const char*
 *
 *   CompressorStats:
 *     - Track cumulative bytes in, bytes out, and call counts via a thread-local
 *       stats accumulator (Reset() clears them).
 */

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Core {

// ── Compression algorithm ─────────────────────────────────────────────────
enum class CompressionAlgo : uint8_t { None, RLE, LZ, LZH };
const char* AlgoName(CompressionAlgo algo);

// ── Compressed blob ───────────────────────────────────────────────────────
struct CompressedBlob {
    std::vector<uint8_t> data;
    size_t               originalSize{0};
    CompressionAlgo      algo{CompressionAlgo::None};
    bool                 valid{false};

    bool IsValid() const { return valid && !data.empty(); }
    float CompressionRatio() const {
        return originalSize > 0
            ? static_cast<float>(data.size()) / static_cast<float>(originalSize)
            : 1.0f;
    }
};

// ── Stats ─────────────────────────────────────────────────────────────────
struct CompressorStats {
    uint64_t totalBytesIn{0};
    uint64_t totalBytesOut{0};
    uint64_t compressCalls{0};
    uint64_t decompressCalls{0};
    float    avgRatio() const {
        return totalBytesIn > 0
            ? static_cast<float>(totalBytesOut) / static_cast<float>(totalBytesIn)
            : 1.0f;
    }
};

// ── DataCompressor ────────────────────────────────────────────────────────
class DataCompressor {
public:
    DataCompressor()  = delete;
    ~DataCompressor() = delete;

    // ── compression ───────────────────────────────────────────
    static CompressedBlob Compress(const uint8_t* data, size_t size,
                                   CompressionAlgo algo = CompressionAlgo::LZ);
    static CompressedBlob CompressString(const std::string& s,
                                         CompressionAlgo algo = CompressionAlgo::LZ);

    // ── decompression ─────────────────────────────────────────
    static std::vector<uint8_t> Decompress(const CompressedBlob& blob);
    static std::string          DecompressString(const CompressedBlob& blob);

    // ── utilities ─────────────────────────────────────────────
    /// Heuristic ratio estimate based on byte-frequency entropy.
    /// Returns expected output/input ratio (< 1 = compressible).
    static float EstimateRatio(const uint8_t* data, size_t size);

    // ── stats ─────────────────────────────────────────────────
    static CompressorStats Stats();
    static void            ResetStats();

private:
    static CompressedBlob CompressRLE(const uint8_t* data, size_t size);
    static CompressedBlob CompressLZ(const uint8_t* data, size_t size);
    static CompressedBlob CompressLZH(const uint8_t* data, size_t size);

    static std::vector<uint8_t> DecompressRLE(const uint8_t* data, size_t size, size_t origSize);
    static std::vector<uint8_t> DecompressLZ(const uint8_t* data, size_t size, size_t origSize);
    static std::vector<uint8_t> DecompressLZH(const uint8_t* data, size_t size, size_t origSize);
};

} // namespace Core
