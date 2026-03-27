#pragma once
/**
 * @file FontSystem.h
 * @brief Font atlas management: glyph cache, text metrics, multi-size, UTF-8.
 *
 * Features:
 *   - Load font faces from TTF/OTF file paths
 *   - Generate glyph atlas textures at arbitrary point sizes (power-of-two packing)
 *   - Glyph cache: UV rect, bearing, advance, size per codepoint
 *   - UTF-8 text decoding
 *   - TextMetrics: MeasureText (width, height, line count for wrapped text)
 *   - SDF (signed distance field) atlas mode for crisp rendering at any scale
 *   - Fallback font chain (try font A, fall back to font B for missing glyphs)
 *   - Line wrapping and truncation helpers
 *   - Atlas texture data exposed as uint8_t array (grayscale or RGBA) for GPU upload
 *   - Multiple faces simultaneously (body text, headings, icons)
 *
 * Typical usage:
 * @code
 *   FontSystem fs;
 *   fs.Init();
 *   uint32_t faceId = fs.LoadFace("Fonts/Roboto-Regular.ttf", {16, 24, 32});
 *   auto& glyph = fs.GetGlyph(faceId, 24, U'A');
 *   auto [w, h]  = fs.MeasureText(faceId, 16, "Hello, World!");
 *   UploadAtlas(fs.GetAtlasData(faceId, 16), fs.GetAtlasWidth(faceId,16), ...);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Engine {

struct GlyphInfo {
    uint32_t codepoint{0};
    // Atlas UV coords (normalised [0,1])
    float    u0{0}, v0{0}, u1{0}, v1{0};
    // Glyph metrics in pixels
    int32_t  width{0}, height{0};
    int32_t  bearingX{0}, bearingY{0};
    int32_t  advance{0};
    bool     valid{false};
};

struct TextMetrics {
    float    width{0};
    float    height{0};
    uint32_t lineCount{1};
};

enum class AtlasMode : uint8_t { Bitmap=0, SDF };

struct FontFaceDesc {
    std::string          path;
    std::vector<uint32_t>sizes;      ///< point sizes to pre-rasterize
    AtlasMode            mode{AtlasMode::Bitmap};
    uint32_t             atlasW{512};
    uint32_t             atlasH{512};
};

class FontSystem {
public:
    FontSystem();
    ~FontSystem();

    void Init();
    void Shutdown();

    // Face management
    uint32_t LoadFace  (const FontFaceDesc& desc);
    uint32_t LoadFace  (const std::string& path, std::vector<uint32_t> sizes={16,24,32});
    void     UnloadFace(uint32_t faceId);
    bool     HasFace   (uint32_t faceId) const;
    void     SetFallback(uint32_t faceId, uint32_t fallbackFaceId);

    // Glyph queries
    const GlyphInfo& GetGlyph(uint32_t faceId, uint32_t ptSize, uint32_t codepoint) const;
    bool             HasGlyph(uint32_t faceId, uint32_t ptSize, uint32_t codepoint) const;

    // Text measurement
    TextMetrics MeasureText(uint32_t faceId, uint32_t ptSize,
                             const std::string& utf8Text,
                             float wrapWidth=0.f) const;

    // Line utilities
    std::string WrapText    (uint32_t faceId, uint32_t ptSize,
                              const std::string& utf8Text, float wrapWidth) const;
    std::string TruncateText(uint32_t faceId, uint32_t ptSize,
                              const std::string& utf8Text, float maxWidth,
                              const std::string& ellipsis="...") const;

    // Atlas access (for GPU upload)
    const uint8_t* GetAtlasData (uint32_t faceId, uint32_t ptSize) const;
    uint32_t       GetAtlasWidth (uint32_t faceId, uint32_t ptSize) const;
    uint32_t       GetAtlasHeight(uint32_t faceId, uint32_t ptSize) const;
    uint32_t       GetAtlasChannels() const; ///< 1 for bitmap, 4 for SDF RGBA

    // On-atlas-rebuild callback (call when GPU texture needs re-upload)
    void SetOnAtlasRebuilt(std::function<void(uint32_t faceId, uint32_t ptSize)> cb);

    // UTF-8 utilities
    static uint32_t DecodeUTF8Codepoint(const char* str, uint32_t& byteLen);
    static std::vector<uint32_t> UTF8ToCodepoints(const std::string& utf8);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
