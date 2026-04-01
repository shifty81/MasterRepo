#pragma once
#include <cstdint>
#include <string>

namespace NF {

/// @brief Pixel format used by a texture or render target.
enum class TextureFormat { RGBA8, RGB8, Depth24Stencil8 };

/// @brief Opaque handle to a GPU texture object.
using TextureId = uint32_t;

/// @brief Manages a 2-D GPU texture resource.
class Texture {
public:
    /// @brief Allocate an empty texture of the given dimensions and format.
    /// @param width  Width in pixels.
    /// @param height Height in pixels.
    /// @param fmt    Pixel format.
    /// @return True on success.
    bool Create(uint32_t width, uint32_t height, TextureFormat fmt);

    /// @brief Load a texture from an image file on disk.
    /// @param path File-system path to the image.
    /// @return True on success.
    bool LoadFromFile(const std::string& path);

    /// @brief Bind this texture to the given texture unit slot.
    /// @param slot Texture unit index (0-based).
    void Bind(uint32_t slot = 0);

    /// @brief Unbind any texture from the given texture unit slot.
    /// @param slot Texture unit index (0-based).
    void Unbind(uint32_t slot = 0);

    /// @brief Return the texture width in pixels.
    [[nodiscard]] uint32_t GetWidth()  const noexcept { return m_Width;  }

    /// @brief Return the texture height in pixels.
    [[nodiscard]] uint32_t GetHeight() const noexcept { return m_Height; }

    /// @brief Return the underlying GPU texture handle.
    [[nodiscard]] TextureId GetId() const noexcept { return m_Id; }

private:
    TextureId     m_Id{0};
    uint32_t      m_Width{0};
    uint32_t      m_Height{0};
    TextureFormat m_Format{TextureFormat::RGBA8};
};

} // namespace NF
