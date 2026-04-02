#include "UI/Rendering/UIRenderer.h"
#include "Core/Logging/Log.h"

#ifdef NF_HAS_OPENGL
#include "Renderer/RHI/GLHeaders.h"
#endif

// stb_easy_font generates quad vertices for text — already in ThirdParty.
#include "stb/stb_easy_font.h"

#include <cstring>

namespace NF {

// ---------------------------------------------------------------------------
// Inline GLSL shaders — simple 2-D coloured-vertex pipeline
// ---------------------------------------------------------------------------

#ifdef NF_HAS_OPENGL
static const char* kUIVertSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;
uniform mat4 uProjection;
out vec4 vColor;
void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    vColor = aColor;
}
)";

static const char* kUIFragSrc = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() {
    FragColor = vColor;
}
)";
#endif

// ---------------------------------------------------------------------------
// Init / Shutdown
// ---------------------------------------------------------------------------

bool UIRenderer::Init() {
#ifdef NF_HAS_OPENGL
    // ---- Compile shader (reusing same pattern as NF::Shader) ----
    auto compileStage = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char buf[512];
            glGetShaderInfoLog(s, sizeof(buf), nullptr, buf);
            NF_LOG_ERROR("UIRenderer", std::string("Shader compile error: ") + buf);
        }
        return s;
    };

    GLuint vert = compileStage(GL_VERTEX_SHADER,   kUIVertSrc);
    GLuint frag = compileStage(GL_FRAGMENT_SHADER, kUIFragSrc);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint linked = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        char buf[512];
        glGetProgramInfoLog(prog, sizeof(buf), nullptr, buf);
        NF_LOG_ERROR("UIRenderer", std::string("Shader link error: ") + buf);
        return false;
    }
    m_ShaderProgram = static_cast<uint32_t>(prog);

    // ---- Allocate VAO / VBO (same pattern as Mesh::Upload) ----
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Position — location 0 — 2 floats
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex),
                          reinterpret_cast<void*>(offsetof(UIVertex, X)));
    // Color — location 1 — 4 floats
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(UIVertex),
                          reinterpret_cast<void*>(offsetof(UIVertex, R)));

    glBindVertexArray(0);

    m_VAO = static_cast<uint32_t>(vao);
    m_VBO = static_cast<uint32_t>(vbo);

    NF_LOG_INFO("UIRenderer", "UIRenderer initialised (OpenGL)");
#else
    NF_LOG_INFO("UIRenderer", "UIRenderer initialised (stub — no OpenGL)");
#endif

    m_Initialised = true;
    return true;
}

void UIRenderer::Shutdown() {
#ifdef NF_HAS_OPENGL
    if (m_VAO) { GLuint v = m_VAO; glDeleteVertexArrays(1, &v); m_VAO = 0; }
    if (m_VBO) { GLuint v = m_VBO; glDeleteBuffers(1, &v);       m_VBO = 0; }
    if (m_ShaderProgram) {
        glDeleteProgram(static_cast<GLuint>(m_ShaderProgram));
        m_ShaderProgram = 0;
    }
#endif
    m_Initialised = false;
    NF_LOG_INFO("UIRenderer", "UIRenderer shut down");
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void UIRenderer::SetViewportSize(float width, float height) {
    m_ViewportWidth  = width;
    m_ViewportHeight = height;
}

void UIRenderer::BeginFrame() {
    m_Vertices.clear();
}

void UIRenderer::DrawRect(const Rect& rect, uint32_t color) {
    const float r = static_cast<float>((color >> 24) & 0xFF) / 255.f;
    const float g = static_cast<float>((color >> 16) & 0xFF) / 255.f;
    const float b = static_cast<float>((color >>  8) & 0xFF) / 255.f;
    const float a = static_cast<float>((color      ) & 0xFF) / 255.f;

    const float x0 = rect.X;
    const float y0 = rect.Y;
    const float x1 = rect.X + rect.Width;
    const float y1 = rect.Y + rect.Height;

    // Two triangles forming a quad (same winding as MeshRenderer).
    m_Vertices.push_back({x0, y0, r, g, b, a});
    m_Vertices.push_back({x1, y0, r, g, b, a});
    m_Vertices.push_back({x1, y1, r, g, b, a});

    m_Vertices.push_back({x0, y0, r, g, b, a});
    m_Vertices.push_back({x1, y1, r, g, b, a});
    m_Vertices.push_back({x0, y1, r, g, b, a});
}

void UIRenderer::DrawOutlineRect(const Rect& rect, uint32_t color) {
    const float t = 1.0f; // 1-pixel border thickness
    // Top
    DrawRect({rect.X, rect.Y, rect.Width, t}, color);
    // Bottom
    DrawRect({rect.X, rect.Y + rect.Height - t, rect.Width, t}, color);
    // Left
    DrawRect({rect.X, rect.Y, t, rect.Height}, color);
    // Right
    DrawRect({rect.X + rect.Width - t, rect.Y, t, rect.Height}, color);
}

void UIRenderer::DrawText(std::string_view text, float x, float y,
                           uint32_t color, float scale) {
    if (text.empty()) return;

    const float r = static_cast<float>((color >> 24) & 0xFF) / 255.f;
    const float g = static_cast<float>((color >> 16) & 0xFF) / 255.f;
    const float b = static_cast<float>((color >>  8) & 0xFF) / 255.f;
    const float a = static_cast<float>((color      ) & 0xFF) / 255.f;

    // stb_easy_font outputs quads (4 verts each) with stride 16 bytes:
    //   float x, float y, float z, uint8_t color[4]
    // We only need x and y; we supply our own colour.
    struct StbVert { float px, py, pz; unsigned char c[4]; };

    // Generous buffer — stb_easy_font uses ~270 bytes per char.
    constexpr int kBufSize = 1024 * 64;
    static char buf[kBufSize]; // static to avoid stack blowup

    // stb_easy_font_print expects a non-const char*
    std::string textCopy(text);
    int numQuads = stb_easy_font_print(0, 0, textCopy.data(), nullptr,
                                       buf, kBufSize);

    const StbVert* verts = reinterpret_cast<const StbVert*>(buf);
    for (int q = 0; q < numQuads; ++q) {
        // Each quad: 4 vertices in order — convert to 2 triangles.
        const StbVert& v0 = verts[q * 4 + 0];
        const StbVert& v1 = verts[q * 4 + 1];
        const StbVert& v2 = verts[q * 4 + 2];
        const StbVert& v3 = verts[q * 4 + 3];

        auto toUI = [&](const StbVert& sv) -> UIVertex {
            return {x + sv.px * scale, y + sv.py * scale, r, g, b, a};
        };

        // Triangle 1: v0, v1, v2
        m_Vertices.push_back(toUI(v0));
        m_Vertices.push_back(toUI(v1));
        m_Vertices.push_back(toUI(v2));

        // Triangle 2: v0, v2, v3
        m_Vertices.push_back(toUI(v0));
        m_Vertices.push_back(toUI(v2));
        m_Vertices.push_back(toUI(v3));
    }
}

void UIRenderer::Flush() {
#ifdef NF_HAS_OPENGL
    if (m_Vertices.empty()) return;

    glUseProgram(static_cast<GLuint>(m_ShaderProgram));

    // Build orthographic projection: (0,0) top-left, (W,H) bottom-right
    float L = 0.f, R2 = m_ViewportWidth;
    float T = 0.f, B  = m_ViewportHeight;
    float proj[16] = {
        2.f / (R2 - L),      0.f,              0.f, 0.f,
        0.f,                  2.f / (T - B),    0.f, 0.f,
        0.f,                  0.f,             -1.f, 0.f,
       -(R2 + L) / (R2 - L), -(T + B) / (T - B), 0.f, 1.f
    };

    GLint loc = glGetUniformLocation(static_cast<GLuint>(m_ShaderProgram), "uProjection");
    glUniformMatrix4fv(loc, 1, GL_FALSE, proj);

    // Upload vertex data
    glBindVertexArray(static_cast<GLuint>(m_VAO));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(m_VBO));
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_Vertices.size() * sizeof(UIVertex)),
                 m_Vertices.data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_Vertices.size()));

    glBindVertexArray(0);
    glUseProgram(0);
#endif
}

void UIRenderer::EndFrame() {
#ifdef NF_HAS_OPENGL
    if (!m_Initialised) return;

    // Save GL state we modify
    GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLint prevBlendSrc = 0, prevBlendDst = 0;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &prevBlendSrc);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &prevBlendDst);

    // Set 2-D rendering state
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Flush();

    // Restore previous GL state
    if (depthWasEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (blendWasEnabled) glEnable(GL_BLEND);      else glDisable(GL_BLEND);
    glBlendFunc(static_cast<GLenum>(prevBlendSrc), static_cast<GLenum>(prevBlendDst));
#endif

    m_Vertices.clear();
}

} // namespace NF
