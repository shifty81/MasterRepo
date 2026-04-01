#include "Renderer/RHI/Texture.h"
#include "Core/Logging/Log.h"

#ifdef NF_HAS_OPENGL
#include "Renderer/RHI/GLHeaders.h"
#endif

namespace NF {

#ifdef NF_HAS_OPENGL
static GLenum ToGLFormat(TextureFormat fmt) {
    switch (fmt) {
    case TextureFormat::RGBA8:            return GL_RGBA;
    case TextureFormat::RGB8:             return GL_RGB;
    case TextureFormat::Depth24Stencil8:  return GL_DEPTH_STENCIL;
    }
    return GL_RGBA;
}

static GLenum ToGLInternalFormat(TextureFormat fmt) {
    switch (fmt) {
    case TextureFormat::RGBA8:            return GL_RGBA8;
    case TextureFormat::RGB8:             return GL_RGB8;
    case TextureFormat::Depth24Stencil8:  return GL_DEPTH24_STENCIL8;
    }
    return GL_RGBA8;
}
#endif

bool Texture::Create(uint32_t width, uint32_t height, TextureFormat fmt) {
    m_Width  = width;
    m_Height = height;
    m_Format = fmt;

#ifdef NF_HAS_OPENGL
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 static_cast<GLint>(ToGLInternalFormat(fmt)),
                 static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                 0, ToGLFormat(fmt), GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    m_Id = static_cast<TextureId>(id);
    NF_LOG_INFO("Renderer", "Texture created");
    return true;
#else
    NF_LOG_WARN("Renderer", "Texture::Create: no OpenGL support");
    return false;
#endif
}

bool Texture::LoadFromFile(const std::string& path) {
    // Actual pixel loading would require an image library (e.g. stb_image).
    // This stub logs and returns false until an image loader is integrated.
    NF_LOG_WARN("Renderer", "Texture::LoadFromFile: image loader not integrated");
    (void)path;
    return false;
}

void Texture::Bind(uint32_t slot) {
#ifdef NF_HAS_OPENGL
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(m_Id));
#else
    (void)slot;
#endif
}

void Texture::Unbind(uint32_t slot) {
#ifdef NF_HAS_OPENGL
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, 0);
#else
    (void)slot;
#endif
}

} // namespace NF
