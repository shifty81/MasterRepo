#include "AssetImporter.h"
#include "../sim/StateHasher.h"
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace atlas::asset {

// ---------------------------------------------------------------------------
// File extension helpers
// ---------------------------------------------------------------------------

static std::string GetLowerExtension(const std::string& filePath) {
    auto ext = std::filesystem::path(filePath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

static std::string StemName(const std::string& filePath) {
    return std::filesystem::path(filePath).stem().string();
}

// ---------------------------------------------------------------------------
// Common import logic
// ---------------------------------------------------------------------------

static ImportResult ImportGeneric(const std::string& filePath,
                                  const ImportOptions& options,
                                  AssetType outputType) {
    ImportResult result;
    result.outputType = outputType;

    if (!std::filesystem::exists(filePath)) {
        result.errorMessage = "Source file not found: " + filePath;
        return result;
    }

    // Read source data
    std::ifstream in(filePath, std::ios::binary | std::ios::ate);
    if (!in.is_open()) {
        result.errorMessage = "Cannot open source file: " + filePath;
        return result;
    }

    auto fileSize = static_cast<size_t>(in.tellg());
    in.seekg(0);
    std::vector<uint8_t> data(fileSize);
    in.read(reinterpret_cast<char*>(data.data()),
            static_cast<std::streamsize>(fileSize));
    in.close();

    // Compute hash of raw payload
    uint64_t hash = sim::StateHasher::HashCombine(0, data.data(), data.size());

    // Determine output path
    std::string outDir = options.outputDirectory.empty()
        ? std::filesystem::path(filePath).parent_path().string()
        : options.outputDirectory;
    std::string outPath = outDir + "/" + StemName(filePath) + ".atlasb";

    // Write .atlasb: header + raw data
    AssetHeader hdr;
    hdr.type = outputType;
    hdr.size = static_cast<uint32_t>(data.size());
    hdr.hash = hash;

    std::ofstream out(outPath, std::ios::binary);
    if (!out.is_open()) {
        result.errorMessage = "Cannot create output file: " + outPath;
        return result;
    }

    out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    out.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));

    if (!out.good()) {
        result.errorMessage = "Write failed: " + outPath;
        return result;
    }

    result.success = true;
    result.outputPath = outPath;
    result.outputHash = hash;
    return result;
}

// ---------------------------------------------------------------------------
// AssetImportRegistry
// ---------------------------------------------------------------------------

void AssetImportRegistry::RegisterImporter(std::unique_ptr<IAssetImporter> importer) {
    if (importer) {
        m_importers.push_back(std::move(importer));
    }
}

IAssetImporter* AssetImportRegistry::FindImporter(const std::string& filePath) const {
    for (const auto& imp : m_importers) {
        if (imp->CanImport(filePath)) return imp.get();
    }
    return nullptr;
}

IAssetImporter* AssetImportRegistry::FindImporterByFormat(ImportFormat format) const {
    for (const auto& imp : m_importers) {
        if (imp->Format() == format) return imp.get();
    }
    return nullptr;
}

ImportResult AssetImportRegistry::Import(const std::string& filePath,
                                          const ImportOptions& options) {
    IAssetImporter* imp = FindImporter(filePath);
    if (!imp) {
        ImportResult r;
        r.errorMessage = "No importer found for: " + filePath;
        return r;
    }
    return imp->Import(filePath, options);
}

std::vector<ImportFormat> AssetImportRegistry::SupportedFormats() const {
    std::vector<ImportFormat> formats;
    formats.reserve(m_importers.size());
    for (const auto& imp : m_importers) {
        formats.push_back(imp->Format());
    }
    return formats;
}

size_t AssetImportRegistry::ImporterCount() const {
    return m_importers.size();
}

// ---------------------------------------------------------------------------
// MeshImporter
// ---------------------------------------------------------------------------

MeshImporter::MeshImporter(ImportFormat format) : m_format(format) {}

ImportFormat MeshImporter::Format() const { return m_format; }

AssetType MeshImporter::OutputType() const { return AssetType::Mesh; }

bool MeshImporter::CanImport(const std::string& filePath) const {
    auto ext = GetLowerExtension(filePath);
    return ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb";
}

ImportResult MeshImporter::Import(const std::string& filePath,
                                   const ImportOptions& options) {
    return ImportGeneric(filePath, options, AssetType::Mesh);
}

// ---------------------------------------------------------------------------
// TextureImporter
// ---------------------------------------------------------------------------

TextureImporter::TextureImporter(ImportFormat format) : m_format(format) {}

ImportFormat TextureImporter::Format() const { return m_format; }

AssetType TextureImporter::OutputType() const { return AssetType::Texture; }

bool TextureImporter::CanImport(const std::string& filePath) const {
    auto ext = GetLowerExtension(filePath);
    return ext == ".png" || ext == ".dds" || ext == ".tga";
}

ImportResult TextureImporter::Import(const std::string& filePath,
                                      const ImportOptions& options) {
    return ImportGeneric(filePath, options, AssetType::Texture);
}

// ---------------------------------------------------------------------------
// AudioImporter
// ---------------------------------------------------------------------------

AudioImporter::AudioImporter(ImportFormat format) : m_format(format) {}

ImportFormat AudioImporter::Format() const { return m_format; }

AssetType AudioImporter::OutputType() const { return AssetType::Audio; }

bool AudioImporter::CanImport(const std::string& filePath) const {
    auto ext = GetLowerExtension(filePath);
    return ext == ".wav" || ext == ".ogg" || ext == ".flac";
}

ImportResult AudioImporter::Import(const std::string& filePath,
                                    const ImportOptions& options) {
    return ImportGeneric(filePath, options, AssetType::Audio);
}

// ---------------------------------------------------------------------------
// FontImporter
// ---------------------------------------------------------------------------

FontImporter::FontImporter(ImportFormat format) : m_format(format) {}

ImportFormat FontImporter::Format() const { return m_format; }

AssetType FontImporter::OutputType() const { return AssetType::Font; }

bool FontImporter::CanImport(const std::string& filePath) const {
    auto ext = GetLowerExtension(filePath);
    return ext == ".ttf" || ext == ".otf";
}

ImportResult FontImporter::Import(const std::string& filePath,
                                   const ImportOptions& options) {
    return ImportGeneric(filePath, options, AssetType::Font);
}

// ---------------------------------------------------------------------------
// SpriteSheetImporter
// ---------------------------------------------------------------------------

ImportFormat SpriteSheetImporter::Format() const { return ImportFormat::SpriteSheet; }

AssetType SpriteSheetImporter::OutputType() const { return AssetType::SpriteSheet; }

bool SpriteSheetImporter::CanImport(const std::string& filePath) const {
    auto ext = GetLowerExtension(filePath);
    return ext == ".sprite" || ext == ".spritesheet";
}

ImportResult SpriteSheetImporter::Import(const std::string& filePath,
                                          const ImportOptions& options) {
    return ImportGeneric(filePath, options, AssetType::SpriteSheet);
}

}  // namespace atlas::asset
