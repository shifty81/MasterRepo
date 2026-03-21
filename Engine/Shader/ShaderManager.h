#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace Engine {

/// A compiled shader object (backend-agnostic handle).
struct ShaderAsset {
    uint32_t    id{0};           ///< Runtime handle (e.g. OpenGL program ID)
    std::string name;            ///< Logical name / key
    std::string vertexSource;    ///< GLSL vertex source (empty if pre-compiled)
    std::string fragmentSource;  ///< GLSL fragment source
    bool        compiled{false};
};

enum class ShaderStage { Vertex, Fragment, Geometry, Compute };

using ShaderID = uint32_t;

/// ShaderManager — shader asset loader and cache.
///
/// Manages the lifetime of all shader programs.  Source code is loaded
/// from disk (or provided inline), compiled on demand, and cached by name.
/// Hot-reload can recompile a shader without restarting the engine.
class ShaderManager {
public:
    ShaderManager();
    ~ShaderManager();

    // ── loading ───────────────────────────────────────────────
    /// Load vertex + fragment sources from file paths and compile.
    ShaderID LoadFromFiles(const std::string& name,
                           const std::string& vertPath,
                           const std::string& fragPath);
    /// Compile from inline GLSL source strings.
    ShaderID LoadFromSource(const std::string& name,
                            const std::string& vertSrc,
                            const std::string& fragSrc);
    /// Reload and recompile a shader by name (hot-reload).
    bool Reload(const std::string& name);
    /// Remove a shader and release its GPU resources.
    bool Unload(const std::string& name);
    void UnloadAll();

    // ── query ─────────────────────────────────────────────────
    ShaderID      GetID(const std::string& name)   const;
    ShaderAsset*  Get(const std::string& name);
    ShaderAsset*  GetByID(ShaderID id);
    bool          Has(const std::string& name)     const;
    size_t        Count()                          const;
    std::vector<std::string> ListNames()           const;

    // ── bind (backend stub — override in derived class) ───────
    virtual void Bind(ShaderID id);
    virtual void Unbind();

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
