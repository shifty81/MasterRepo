#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace Tools {

// ──────────────────────────────────────────────────────────────
// Asset type identifiers
// ──────────────────────────────────────────────────────────────

enum class AssetType : uint32_t {
    Unknown  = 0,
    Mesh     = 1,
    Texture  = 2,
    Audio    = 3,
    Material = 4,
    Animation= 5,
    Scene    = 6,
    Font     = 7,
    Script   = 8,
    Binary   = 99
};

// ──────────────────────────────────────────────────────────────
// Atlas binary asset header (.atlasb)
// ──────────────────────────────────────────────────────────────

struct AssetHeader {
    char        magic[4]  = {'A','T','L','B'};
    uint32_t    version   = 1;
    AssetType   type      = AssetType::Unknown;
    uint32_t    size      = 0;   // payload bytes
    uint64_t    hash      = 0;   // FNV-1a of payload
    char        name[64]  = {};
};

// ──────────────────────────────────────────────────────────────
// Import options
// ──────────────────────────────────────────────────────────────

struct ImportOptions {
    std::string outputDirectory;   // empty = same dir as source
    bool        overwrite       = true;
    bool        generateMipmaps = false;  // textures
    float       meshScale       = 1.0f;   // meshes
    bool        flipUVs         = false;  // meshes/textures
};

// ──────────────────────────────────────────────────────────────
// Import result
// ──────────────────────────────────────────────────────────────

struct ImportResult {
    bool        success      = false;
    AssetType   outputType   = AssetType::Unknown;
    std::string outputPath;
    std::string errorMessage;
    uint64_t    hash         = 0;
    uint32_t    payloadBytes = 0;
};

// ──────────────────────────────────────────────────────────────
// AssetImporter — migrated from atlas::asset
// ──────────────────────────────────────────────────────────────

class AssetImporter {
public:
    // Import a file; type is inferred from extension
    static ImportResult Import(const std::string& filePath,
                               const ImportOptions& options = {});

    // Explicit type imports
    static ImportResult ImportMesh(const std::string& filePath,
                                   const ImportOptions& options = {});
    static ImportResult ImportTexture(const std::string& filePath,
                                      const ImportOptions& options = {});
    static ImportResult ImportAudio(const std::string& filePath,
                                    const ImportOptions& options = {});

    // Batch import all assets in a directory
    static std::vector<ImportResult> ImportDirectory(const std::string& dirPath,
                                                      const ImportOptions& options = {},
                                                      bool recursive = false);

    // Load a previously exported .atlasb header (payload not loaded)
    static bool ReadHeader(const std::string& atlasbPath, AssetHeader& header);

    // Determine asset type from file extension
    static AssetType InferType(const std::string& filePath);

    // Supported extensions
    static bool IsSupportedMesh(const std::string& ext);
    static bool IsSupportedTexture(const std::string& ext);
    static bool IsSupportedAudio(const std::string& ext);

private:
    static uint64_t HashPayload(const uint8_t* data, size_t size);
    static std::string StemName(const std::string& filePath);
    static std::string LowerExtension(const std::string& filePath);
    static ImportResult WriteAtlasb(const std::string& outPath, AssetType type,
                                    const std::string& name,
                                    const std::vector<uint8_t>& payload);
};

} // namespace Tools
