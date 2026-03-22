#pragma once
/**
 * @file TextureAtlasPacker.h
 * @brief Texture atlas packer using shelf-based and MaxRects bin-packing algorithms.
 *
 * TextureAtlasPacker takes a set of named textures (width × height pixels) and
 * packs them into one or more atlas pages of a fixed size:
 *
 *   PackAlgo: Shelf (fast, moderate waste), MaxRects (slower, near-optimal).
 *   AtlasConfig: pageWidth, pageHeight, padding (pixels between sprites),
 *                allowRotation, algo, maxPages.
 *
 *   TextureEntry:  name, width, height (input).
 *   PackedTexture: name, pageIndex, x, y, width, height, rotated (output).
 *   AtlasPage:     pageIndex, list of PackedTexture, utilisation (%).
 *   PackResult:    pages, unpacked (names that didn't fit), stats.
 *
 *   TextureAtlasPacker:
 *     - AddTexture(entry): add an input texture.
 *     - RemoveTexture(name): remove an input texture.
 *     - ClearTextures(): remove all inputs.
 *     - Pack(config): run packing; returns PackResult.
 *     - GetPackedTexture(name): look up result by name.
 *     - ExportManifestJSON(result): generate atlas manifest as JSON string.
 *     - AtlasStats: inputCount, packedCount, pageCount, avgUtilisation, packTimeMs.
 */

#include <cstdint>
#include <string>
#include <vector>

namespace Tools {

// ── Packing algorithm ─────────────────────────────────────────────────────
enum class PackAlgo : uint8_t { Shelf, MaxRects };

// ── Atlas configuration ───────────────────────────────────────────────────
struct AtlasConfig {
    uint32_t  pageWidth{2048};
    uint32_t  pageHeight{2048};
    uint32_t  padding{2};           ///< Pixels between sprites
    bool      allowRotation{true};
    PackAlgo  algo{PackAlgo::MaxRects};
    uint32_t  maxPages{8};
};

// ── Input texture ─────────────────────────────────────────────────────────
struct TextureEntry {
    std::string name;
    uint32_t    width{0};
    uint32_t    height{0};
};

// ── Packed texture (output) ───────────────────────────────────────────────
struct PackedTexture {
    std::string name;
    uint32_t    pageIndex{0};
    uint32_t    x{0};
    uint32_t    y{0};
    uint32_t    width{0};
    uint32_t    height{0};
    bool        rotated{false};     ///< True if 90° clockwise rotation applied
};

// ── Atlas page ────────────────────────────────────────────────────────────
struct AtlasPage {
    uint32_t                   pageIndex{0};
    std::vector<PackedTexture> packed;
    float                      utilisation{0.0f}; ///< [0, 1]
};

// ── Stats ─────────────────────────────────────────────────────────────────
struct AtlasStats {
    uint32_t inputCount{0};
    uint32_t packedCount{0};
    uint32_t unpackedCount{0};
    uint32_t pageCount{0};
    float    avgUtilisation{0.0f};
    double   packTimeMs{0.0};
};

// ── Pack result ───────────────────────────────────────────────────────────
struct PackResult {
    std::vector<AtlasPage>  pages;
    std::vector<std::string> unpacked; ///< Textures that could not be placed
    AtlasStats               stats;
    bool                     success{false};
};

// ── TextureAtlasPacker ────────────────────────────────────────────────────
class TextureAtlasPacker {
public:
    TextureAtlasPacker() = default;
    ~TextureAtlasPacker() = default;

    TextureAtlasPacker(const TextureAtlasPacker&) = delete;
    TextureAtlasPacker& operator=(const TextureAtlasPacker&) = delete;

    // ── input management ──────────────────────────────────────
    void AddTexture(const TextureEntry& entry);
    bool RemoveTexture(const std::string& name);
    void ClearTextures();
    size_t TextureCount() const;

    // ── packing ───────────────────────────────────────────────
    PackResult Pack(const AtlasConfig& config);

    // ── result queries ────────────────────────────────────────
    const PackedTexture* GetPackedTexture(const std::string& name) const;

    // ── export ────────────────────────────────────────────────
    std::string ExportManifestJSON(const PackResult& result) const;

    // ── stats ─────────────────────────────────────────────────
    AtlasStats GetLastStats() const;

private:
    std::vector<TextureEntry> m_textures;
    PackResult                m_lastResult;

    PackResult PackShelf   (const AtlasConfig& cfg,
                            std::vector<TextureEntry> sorted) const;
    PackResult PackMaxRects(const AtlasConfig& cfg,
                            std::vector<TextureEntry> sorted) const;

    static float PageUtil(const AtlasPage& page,
                          uint32_t pageW, uint32_t pageH);
};

} // namespace Tools
