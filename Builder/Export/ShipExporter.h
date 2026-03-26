#pragma once
/**
 * @file ShipExporter.h
 * @brief Export assembled ships and stations from the Builder system to
 *        standard interchange formats (JSON blueprint, OBJ mesh, PAK bundle).
 *
 * Typical usage:
 * @code
 *   ShipExporter exporter;
 *   exporter.Init();
 *   exporter.LoadAssembly("Saves/build_session_0.json");
 *   exporter.ExportBlueprint("Export/Cruiser.json");
 *   exporter.ExportOBJ("Export/Cruiser.obj");
 *   exporter.ExportPAK("Export/Cruiser.pak");
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Builder {

// ── Export format selector ────────────────────────────────────────────────────

enum class ExportFormat : uint8_t {
    Blueprint = 0,  ///< JSON part-list + snap-graph (reimportable)
    OBJ       = 1,  ///< merged Wavefront OBJ + MTL for external tools
    PAK       = 2,  ///< binary asset bundle (.pak) for in-game streaming
    GLB       = 3,  ///< glTF 2.0 binary for preview / external engines
};

// ── Result of an export operation ────────────────────────────────────────────

struct ExportResult {
    bool        succeeded{false};
    std::string outputPath;
    uint32_t    partCount{0};
    uint64_t    fileSizeBytes{0};
    std::string errorMessage;
    uint64_t    durationMs{0};
};

// ── Export options ────────────────────────────────────────────────────────────

struct ExportOptions {
    bool mergeByMaterial{true};    ///< OBJ/GLB: merge parts sharing same material
    bool includeCollision{false};  ///< PAK: bundle collision shapes
    bool includeInterior{true};    ///< include interior nodes
    bool compressTextures{true};   ///< PAK/GLB: compress embedded textures
    std::string authorTag;         ///< embedded in metadata
    std::string versionTag;
};

// ── ShipExporter ──────────────────────────────────────────────────────────────

class ShipExporter {
public:
    ShipExporter();
    ~ShipExporter();

    void Init();
    void Shutdown();

    // ── Source data ───────────────────────────────────────────────────────────

    /// Load an assembly from a saved build-session JSON file.
    bool LoadAssembly(const std::string& sessionPath);

    /// Load directly from a raw JSON string.
    bool LoadAssemblyFromJSON(const std::string& jsonString);

    bool IsAssemblyLoaded() const;
    std::string AssemblyName() const;
    uint32_t    PartCount()    const;

    // ── Export ────────────────────────────────────────────────────────────────

    ExportResult Export(const std::string& outputPath, ExportFormat format,
                        const ExportOptions& options = {});

    ExportResult ExportBlueprint(const std::string& outputPath,
                                 const ExportOptions& options = {});
    ExportResult ExportOBJ(const std::string& outputPath,
                           const ExportOptions& options = {});
    ExportResult ExportPAK(const std::string& outputPath,
                           const ExportOptions& options = {});
    ExportResult ExportGLB(const std::string& outputPath,
                           const ExportOptions& options = {});

    // ── Batch export ──────────────────────────────────────────────────────────

    /// Export all session files found in a directory.
    std::vector<ExportResult> BatchExport(const std::string& sessionDir,
                                          const std::string& outputDir,
                                          ExportFormat format,
                                          const ExportOptions& options = {});

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnExportComplete(std::function<void(const ExportResult&)> cb);
    void OnExportError(std::function<void(const std::string& msg)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Builder
