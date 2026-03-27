#pragma once
/**
 * @file AssetImporter.h
 * @brief Asset import pipeline: mesh, texture, audio, JSON — metadata, dependencies, caching.
 *
 * Features:
 *   - Asset types: Mesh, Texture, Audio, JSON, Script, Binary (extensible)
 *   - Import pipeline pass: source file → processed in-memory asset record
 *   - Dependency tracking: asset A depends on B (invalidation propagation)
 *   - Content hash fingerprinting: skip re-import if source unchanged
 *   - Per-type importer plugins (register custom ImportFn)
 *   - Import settings struct per asset type (e.g., texture compression, mesh normalise)
 *   - Async import queue with priority and progress callback
 *   - Import result: success/failure, warnings, imported asset path
 *   - Asset manifest: JSON index of all imported assets with hashes
 *   - Hot-reload: watch source directory for changes and re-import
 *
 * Typical usage:
 * @code
 *   AssetImporter ai;
 *   ai.Init("Assets/", "Cache/");
 *   ai.RegisterImporter(AssetType::Texture, MyTextureImporter);
 *   auto result = ai.Import("Assets/logo.png");
 *   ai.ImportAsync("Assets/level.fbx", [](const ImportResult& r){ OnLoaded(r); });
 *   ai.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class AssetType : uint8_t {
    Unknown=0, Mesh, Texture, Audio, JSON, Script, Binary, Font, Material
};

struct ImportSettings {
    AssetType   type        {AssetType::Unknown};
    bool        generateMips{true};   ///< Texture: auto mipmap
    bool        normalise   {true};   ///< Mesh: centre & unit scale
    uint32_t    targetWidth {0};      ///< Texture: 0=keep original
    uint32_t    targetHeight{0};
    bool        compress    {false};  ///< Texture: compress to GPU format
    float       volumeNorm  {0.f};   ///< Audio: 0=skip normalise
    std::string customArgs;           ///< passthrough to plugin
};

struct AssetRecord {
    std::string id;             ///< canonical asset id
    std::string sourcePath;
    std::string cachedPath;
    AssetType   type    {AssetType::Unknown};
    uint64_t    sourceHash{0};
    uint64_t    importTimestamp{0};
    std::vector<std::string> dependencies;
    bool        valid{false};
};

struct ImportResult {
    bool        success{false};
    std::string assetId;
    std::string cachedPath;
    std::string errorMsg;
    std::vector<std::string> warnings;
    AssetRecord record;
};

using ImportFn   = std::function<ImportResult(const std::string& sourcePath,
                                               const ImportSettings& settings,
                                               const std::string& cacheDir)>;
using ProgressFn = std::function<void(const std::string& assetId, float progress)>;

class AssetImporter {
public:
    AssetImporter();
    ~AssetImporter();

    void Init    (const std::string& sourceRoot, const std::string& cacheRoot);
    void Shutdown();
    void Tick    (float dt);

    // Importers
    void RegisterImporter  (AssetType type, ImportFn fn);
    void UnregisterImporter(AssetType type);

    // Synchronous import
    ImportResult Import(const std::string& sourcePath,
                        const ImportSettings& settings={});
    ImportResult ImportById(const std::string& assetId);

    // Async import
    void ImportAsync(const std::string& sourcePath,
                     std::function<void(const ImportResult&)> onDone,
                     ProgressFn onProgress={},
                     const ImportSettings& settings={});
    void CancelAsync(const std::string& sourcePath);

    // Asset registry
    bool               HasAsset       (const std::string& assetId) const;
    const AssetRecord* GetRecord      (const std::string& assetId) const;
    std::vector<AssetRecord> GetAll   ()                           const;
    std::vector<AssetRecord> GetByType(AssetType type)             const;

    // Dependency
    std::vector<std::string> GetDependents(const std::string& assetId) const;
    void InvalidateAsset(const std::string& assetId, bool cascade=true);

    // Manifest
    bool SaveManifest(const std::string& path) const;
    bool LoadManifest(const std::string& path);

    // Hot-reload
    void SetHotReload(bool enabled);
    void ScanForChanges();

    // Detect asset type from extension
    static AssetType DetectType(const std::string& path);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
