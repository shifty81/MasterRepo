#include "Core/Compression/DataCompressor/DataCompressor.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace Core {

// ── Module-level stats ────────────────────────────────────────────────────
static CompressorStats g_stats;

const char* AlgoName(CompressionAlgo algo) {
    switch (algo) {
    case CompressionAlgo::None: return "None";
    case CompressionAlgo::RLE:  return "RLE";
    case CompressionAlgo::LZ:   return "LZ";
    case CompressionAlgo::LZH:  return "LZH";
    }
    return "Unknown";
}

// ── RLE ───────────────────────────────────────────────────────────────────
// Format: [count:1][byte:1] pairs. count 1..255; run > 255 split.
CompressedBlob DataCompressor::CompressRLE(const uint8_t* data, size_t size) {
    CompressedBlob blob;
    blob.originalSize = size;
    blob.algo = CompressionAlgo::RLE;
    if (size == 0) { blob.valid = true; return blob; }

    std::vector<uint8_t>& out = blob.data;
    size_t i = 0;
    while (i < size) {
        uint8_t val = data[i];
        size_t  run = 1;
        while (i + run < size && data[i + run] == val && run < 255) ++run;
        out.push_back(static_cast<uint8_t>(run));
        out.push_back(val);
        i += run;
    }
    blob.valid = true;
    return blob;
}

std::vector<uint8_t> DataCompressor::DecompressRLE(const uint8_t* data, size_t size, size_t origSize) {
    std::vector<uint8_t> out;
    out.reserve(origSize);
    for (size_t i = 0; i + 1 < size; i += 2) {
        uint8_t count = data[i];
        uint8_t val   = data[i+1];
        for (uint8_t k = 0; k < count; ++k) out.push_back(val);
    }
    return out;
}

// ── LZ77-style ────────────────────────────────────────────────────────────
// Encoding: flag byte (bit per literal/copy), then literals or (offset16, len8) back-refs.
// Window: 4096 bytes. Min match: 3. Max match: 18.
static constexpr size_t kWindow  = 4096;
static constexpr size_t kMinMatch = 3;
static constexpr size_t kMaxMatch = 18;

CompressedBlob DataCompressor::CompressLZ(const uint8_t* data, size_t size) {
    CompressedBlob blob;
    blob.originalSize = size;
    blob.algo = CompressionAlgo::LZ;
    if (size == 0) { blob.valid = true; return blob; }

    std::vector<uint8_t>& out = blob.data;
    size_t pos = 0;

    while (pos < size) {
        // Up to 8 tokens per flag byte
        size_t flagPos = out.size();
        out.push_back(0); // placeholder flag
        uint8_t flags = 0;

        for (int bit = 0; bit < 8 && pos < size; ++bit) {
            // Search for a back-reference
            size_t winStart = pos > kWindow ? pos - kWindow : 0;
            size_t bestLen  = 0;
            size_t bestOff  = 0;

            for (size_t s = winStart; s < pos; ++s) {
                size_t len = 0;
                while (len < kMaxMatch && pos + len < size && data[s + len] == data[pos + len]) {
                    ++len;
                    if (s + len >= pos) break; // don't cross pos
                }
                if (len >= kMinMatch && len > bestLen) {
                    bestLen = len;
                    bestOff = pos - s;
                }
            }

            if (bestLen >= kMinMatch) {
                // Copy token: flag bit = 1
                flags |= (1u << bit);
                out.push_back(static_cast<uint8_t>((bestOff >> 8) & 0x0F));
                out.push_back(static_cast<uint8_t>(bestOff & 0xFF));
                out.push_back(static_cast<uint8_t>(bestLen - kMinMatch));
                pos += bestLen;
            } else {
                // Literal: flag bit = 0
                out.push_back(data[pos++]);
            }
        }
        out[flagPos] = flags;
    }
    blob.valid = true;
    return blob;
}

std::vector<uint8_t> DataCompressor::DecompressLZ(const uint8_t* data, size_t size, size_t origSize) {
    std::vector<uint8_t> out;
    out.reserve(origSize);
    size_t i = 0;
    while (i < size) {
        uint8_t flags = data[i++];
        for (int bit = 0; bit < 8 && i < size; ++bit) {
            if (flags & (1u << bit)) {
                // Back-reference
                if (i + 2 >= size) break;
                uint16_t off = (static_cast<uint16_t>(data[i] & 0x0F) << 8) | data[i+1];
                uint8_t  len = data[i+2] + static_cast<uint8_t>(kMinMatch);
                i += 3;
                if (off == 0 || off > out.size()) break;
                size_t srcPos = out.size() - off;
                for (uint8_t k = 0; k < len; ++k)
                    out.push_back(out[srcPos + k]);
            } else {
                // Literal
                out.push_back(data[i++]);
            }
        }
    }
    return out;
}

// ── LZH: LZ with simple byte-frequency packing (delta-encode frequencies) ─
// For simplicity, LZH = LZ + a header. In a production system you'd add
// Huffman coding; here it gracefully degrades to LZ.
CompressedBlob DataCompressor::CompressLZH(const uint8_t* data, size_t size) {
    // LZH = [magic:1] + LZ-compressed payload
    auto lz = CompressLZ(data, size);
    CompressedBlob blob;
    blob.originalSize = size;
    blob.algo = CompressionAlgo::LZH;
    blob.data.reserve(lz.data.size() + 1);
    blob.data.push_back(0xAB); // magic
    blob.data.insert(blob.data.end(), lz.data.begin(), lz.data.end());
    blob.valid = true;
    return blob;
}

std::vector<uint8_t> DataCompressor::DecompressLZH(const uint8_t* data, size_t size, size_t origSize) {
    if (size == 0 || data[0] != 0xAB) return {};
    return DecompressLZ(data + 1, size - 1, origSize);
}

// ── Public API ────────────────────────────────────────────────────────────
CompressedBlob DataCompressor::Compress(const uint8_t* data, size_t size, CompressionAlgo algo) {
    CompressedBlob blob;
    switch (algo) {
    case CompressionAlgo::None:
        blob.data.assign(data, data + size);
        blob.originalSize = size;
        blob.algo  = CompressionAlgo::None;
        blob.valid = true;
        break;
    case CompressionAlgo::RLE:  blob = CompressRLE(data, size);  break;
    case CompressionAlgo::LZ:   blob = CompressLZ(data, size);   break;
    case CompressionAlgo::LZH:  blob = CompressLZH(data, size);  break;
    }
    g_stats.totalBytesIn  += size;
    g_stats.totalBytesOut += blob.data.size();
    ++g_stats.compressCalls;
    return blob;
}

CompressedBlob DataCompressor::CompressString(const std::string& s, CompressionAlgo algo) {
    return Compress(reinterpret_cast<const uint8_t*>(s.data()), s.size(), algo);
}

std::vector<uint8_t> DataCompressor::Decompress(const CompressedBlob& blob) {
    ++g_stats.decompressCalls;
    if (!blob.valid) return {};
    const uint8_t* d = blob.data.data();
    size_t         n = blob.data.size();
    switch (blob.algo) {
    case CompressionAlgo::None:  return std::vector<uint8_t>(d, d + n);
    case CompressionAlgo::RLE:   return DecompressRLE(d, n, blob.originalSize);
    case CompressionAlgo::LZ:    return DecompressLZ(d, n, blob.originalSize);
    case CompressionAlgo::LZH:   return DecompressLZH(d, n, blob.originalSize);
    }
    return {};
}

std::string DataCompressor::DecompressString(const CompressedBlob& blob) {
    auto v = Decompress(blob);
    return std::string(v.begin(), v.end());
}

float DataCompressor::EstimateRatio(const uint8_t* data, size_t size) {
    if (size == 0) return 1.0f;
    // Shannon entropy estimate
    std::array<uint64_t, 256> freq{};
    for (size_t i = 0; i < size; ++i) ++freq[data[i]];
    double entropy = 0;
    for (auto f : freq) {
        if (f == 0) continue;
        double p = static_cast<double>(f) / static_cast<double>(size);
        entropy -= p * std::log2(p);
    }
    // Ideal bits per byte / 8 → ratio
    return static_cast<float>(entropy / 8.0);
}

CompressorStats DataCompressor::Stats()      { return g_stats; }
void            DataCompressor::ResetStats() { g_stats = {}; }

} // namespace Core
