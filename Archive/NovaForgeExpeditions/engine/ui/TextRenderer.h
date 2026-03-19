#pragma once
#include "UIRenderer.h"
#include <cstdint>
#include <string>
#include <unordered_map>

namespace atlas::ui {

/// Glyph metrics for a single character in a font atlas.
struct Glyph {
    float x = 0.0f;       ///< Atlas X position
    float y = 0.0f;       ///< Atlas Y position
    float w = 0.0f;       ///< Glyph width in atlas
    float h = 0.0f;       ///< Glyph height in atlas
    float xOffset = 0.0f; ///< Horizontal bearing
    float yOffset = 0.0f; ///< Vertical bearing
    float advance = 0.0f; ///< Horizontal advance
};

/// A font atlas containing glyph metrics and a texture handle.
struct FontAtlas {
    uint32_t textureId = 0;
    float fontSize = 0.0f;
    float lineHeight = 0.0f;
    std::unordered_map<uint32_t, Glyph> glyphs;
};

using TextFontHandle = uint32_t;

/// Backend-agnostic text rendering interface.
/// Concrete implementations (DX11, Vulkan, OpenGL) derive from this
/// and handle GPU-specific texture upload and quad drawing.
class TextRenderer {
public:
    virtual ~TextRenderer() = default;

    /// Load a font atlas from an offline-generated JSON + texture pair.
    virtual TextFontHandle LoadFontAtlas(const std::string& jsonPath) = 0;

    /// Destroy a previously loaded font.
    virtual void DestroyFont(TextFontHandle handle) = 0;

    /// Returns true if the handle refers to a valid, loaded font.
    virtual bool IsValidFont(TextFontHandle handle) const = 0;

    /// Rebuild the font texture after a renderer reset.
    virtual void RebuildFontTexture(TextFontHandle handle) = 0;

    /// Draw a string at the given position with the specified colour.
    virtual void DrawText(TextFontHandle font,
                          const char* text,
                          float x, float y,
                          const UIColor& color) = 0;

    /// Measure the pixel width of a string without drawing it.
    virtual float MeasureText(TextFontHandle font,
                              const char* text) const = 0;

    /// Begin / end a text rendering batch for the current frame.
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
};

/// Null implementation used in headless / server mode.
class NullTextRenderer : public TextRenderer {
public:
    TextFontHandle LoadFontAtlas(const std::string& /*jsonPath*/) override { return 0; }
    void DestroyFont(TextFontHandle /*handle*/) override {}
    bool IsValidFont(TextFontHandle /*handle*/) const override { return false; }
    void RebuildFontTexture(TextFontHandle /*handle*/) override {}
    void DrawText(TextFontHandle /*font*/, const char* /*text*/,
                  float /*x*/, float /*y*/,
                  const UIColor& /*color*/) override {}
    float MeasureText(TextFontHandle /*font*/,
                      const char* /*text*/) const override { return 0.0f; }
    void BeginFrame() override {}
    void EndFrame() override {}
};

} // namespace atlas::ui
