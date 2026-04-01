#include "Renderer/Mesh/MeshRenderer.h"
#include "Core/Logging/Log.h"

#ifdef NF_HAS_OPENGL
#include "Renderer/RHI/GLHeaders.h"
#endif

namespace NF {

void Mesh::Upload(const MeshData& data) {
    m_VertexCount = static_cast<uint32_t>(data.Vertices.size());
    m_IndexCount  = static_cast<uint32_t>(data.Indices.size());

#ifdef NF_HAS_OPENGL
    // Clean up any previous buffers.
    if (m_VAO) { glDeleteVertexArrays(1, &m_VAO); m_VAO = 0; }
    if (m_VBO) { glDeleteBuffers(1, &m_VBO);       m_VBO = 0; }
    if (m_IBO) { glDeleteBuffers(1, &m_IBO);       m_IBO = 0; }

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_IBO);

    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(data.Vertices.size() * sizeof(Vertex)),
                 data.Vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(data.Indices.size() * sizeof(uint32_t)),
                 data.Indices.data(), GL_STATIC_DRAW);

    // Position (location 0) – 3 floats
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, Position)));
    // Normal (location 1) – 3 floats
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, Normal)));
    // TexCoord (location 2) – 2 floats
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, TexCoord)));

    glBindVertexArray(0);
    NF_LOG_INFO("Renderer", "Mesh uploaded to GPU");
#else
    NF_LOG_WARN("Renderer", "Mesh::Upload: no OpenGL support");
#endif
}

void Mesh::Bind() {
#ifdef NF_HAS_OPENGL
    glBindVertexArray(m_VAO);
#endif
}

void Mesh::Draw() {
#ifdef NF_HAS_OPENGL
    if (m_IndexCount > 0) {
        glDrawElements(GL_TRIANGLES,
                       static_cast<GLsizei>(m_IndexCount),
                       GL_UNSIGNED_INT, nullptr);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_VertexCount));
    }
#endif
}

} // namespace NF
