#include "Renderer/RHI/Shader.h"
#include "Core/Logging/Log.h"

#ifdef NF_HAS_OPENGL
#include "Renderer/RHI/GLHeaders.h"
#endif

namespace NF {

bool Shader::LoadFromSource(const std::string& vertSrc, const std::string& fragSrc) {
#ifdef NF_HAS_OPENGL
    auto compile = [](GLenum type, const std::string& src) -> GLuint {
        GLuint shader = glCreateShader(type);
        const char* c = src.c_str();
        glShaderSource(shader, 1, &c, nullptr);
        glCompileShader(shader);
        return shader;
    };

    GLuint vert = compile(GL_VERTEX_SHADER,   vertSrc);
    GLuint frag = compile(GL_FRAGMENT_SHADER, fragSrc);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    glDeleteShader(vert);
    glDeleteShader(frag);

    m_Id = static_cast<ShaderId>(prog);
    NF_LOG_INFO("Renderer", "Shader compiled and linked");
    return true;
#else
    (void)vertSrc; (void)fragSrc;
    NF_LOG_WARN("Renderer", "Shader::LoadFromSource: no OpenGL support");
    return false;
#endif
}

void Shader::Bind() {
#ifdef NF_HAS_OPENGL
    glUseProgram(static_cast<GLuint>(m_Id));
#endif
}

void Shader::Unbind() {
#ifdef NF_HAS_OPENGL
    glUseProgram(0);
#endif
}

void Shader::SetInt(const std::string& name, int val) {
#ifdef NF_HAS_OPENGL
    glUniform1i(glGetUniformLocation(static_cast<GLuint>(m_Id), name.c_str()), val);
#else
    (void)name; (void)val;
#endif
}

void Shader::SetFloat(const std::string& name, float val) {
#ifdef NF_HAS_OPENGL
    glUniform1f(glGetUniformLocation(static_cast<GLuint>(m_Id), name.c_str()), val);
#else
    (void)name; (void)val;
#endif
}

void Shader::SetVec3(const std::string& name, const Vector3& val) {
#ifdef NF_HAS_OPENGL
    glUniform3f(glGetUniformLocation(static_cast<GLuint>(m_Id), name.c_str()),
                val.X, val.Y, val.Z);
#else
    (void)name; (void)val;
#endif
}

void Shader::SetMat4(const std::string& name, const Matrix4x4& val) {
#ifdef NF_HAS_OPENGL
    glUniformMatrix4fv(
        glGetUniformLocation(static_cast<GLuint>(m_Id), name.c_str()),
        1, GL_FALSE, &val.M[0][0]);
#else
    (void)name; (void)val;
#endif
}

} // namespace NF
