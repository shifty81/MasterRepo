#pragma once
/**
 * @file TextureAtlasBuilder.h
 * @brief Builds texture atlases (sprite sheets) from individual image regions.
 *
 * Packs N RGBA sub-textures into a single power-of-2 atlas texture using
 * a shelf-first bin-packing algorithm.  Output is a flat RGBA pixel buffer
 * plus an atlas map (sub-texture name → UV rect).
 *
 * Features:
 *   - Add from file paths or raw RGBA pixel buffers
 *   - Optional padding between sub-textures (prevents bleeding)
 *   - Power-of-2 or exact-fit atlas sizes
 *   - Atlas map export (JSON UV coordinates)
 *   - PNG export of the packed atlas
 *
 * Typical usage:
 * @code
 *   TextureAtlasBuilder tab;
 *   tab.Init(2048, 2048, 2);
 *   tab.AddFromBuffer("player_idle", pixels, 64, 64);
 *   tab.AddFromBuffer("player_run",  pixels2, 64, 64);
 *   bool ok = tab.Build();
 *   UVRect uv = tab.GetUV("player_idle");
 *   tab.ExportAtlas("Assets/atlas.png");
 *   tab.ExportMap("Assets/atlas.json");
 * @endcode
 */

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace Engine {

struct UVRect {
    float u{0.f}, v{0.f};   ///< top-left (0-1)
    float w{0.f}, h{0.f};   ///< width, height (0-1)
    uint32_t pixelX{0}, pixelY{0};
    uint32_t pixelW{0}, pixelH{0};
};

struct AtlasEntry {
    std::string           name;
    std::vector<uint8_t>  rgba;   ///< width*height*4
    uint32_t              width{0}, height{0};
    UVRect                uv;
    bool                  packed{false};
};

class TextureAtlasBuilder {
public:
    TextureAtlasBuilder();
    ~TextureAtlasBuilder();

    void Init(uint32_t atlasWidth = 2048, uint32_t atlasHeight = 2048,
              uint32_t padding = 1);
    void Clear();

    bool AddFromFile(const std::string& name, const std::string& path);
    bool AddFromBuffer(const std::string& name,
                       const std::vector<uint8_t>& rgba,
                       uint32_t width, uint32_t height);

    bool Build();

    bool   HasEntry(const std::string& name) const;
    UVRect GetUV(const std::string& name)    const;
    std::unordered_map<std::string,UVRect> AllUVs() const;

    const std::vector<uint8_t>& GetAtlasPixels() const;
    uint32_t GetAtlasWidth()  const;
    uint32_t GetAtlasHeight() const;

    bool ExportAtlas(const std::string& pngPath)  const;
    bool ExportMap(const std::string& jsonPath)   const;

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
