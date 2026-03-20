#include "Tools/Importer/AssetImporter.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace Tools {

// ──────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────

std::string AssetImporter::LowerExtension(const std::string& filePath) {
    auto ext = std::filesystem::path(filePath).extension().string();
    for (char& c : ext) c = (char)std::tolower((unsigned char)c);
    return ext;
}

std::string AssetImporter::StemName(const std::string& filePath) {
    return std::filesystem::path(filePath).stem().string();
}

uint64_t AssetImporter::HashPayload(const uint8_t* data, size_t size) {
    // FNV-1a 64-bit
    uint64_t hash = 14695981039346656037ULL;
    for (size_t i = 0; i < size; ++i) {
        hash ^= (uint64_t)data[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

bool AssetImporter::IsSupportedMesh(const std::string& ext) {
    return ext==".obj"||ext==".fbx"||ext==".gltf"||ext==".glb"||ext==".dae"||ext==".stl";
}

bool AssetImporter::IsSupportedTexture(const std::string& ext) {
    return ext==".png"||ext==".jpg"||ext==".jpeg"||ext==".bmp"||ext==".tga"||
           ext==".hdr"||ext==".exr"||ext==".ppm";
}

bool AssetImporter::IsSupportedAudio(const std::string& ext) {
    return ext==".wav"||ext==".ogg"||ext==".mp3"||ext==".flac"||ext==".aiff";
}

AssetType AssetImporter::InferType(const std::string& filePath) {
    std::string ext = LowerExtension(filePath);
    if (IsSupportedMesh(ext))    return AssetType::Mesh;
    if (IsSupportedTexture(ext)) return AssetType::Texture;
    if (IsSupportedAudio(ext))   return AssetType::Audio;
    if (ext==".mat")             return AssetType::Material;
    if (ext==".anim")            return AssetType::Animation;
    if (ext==".scene")           return AssetType::Scene;
    if (ext==".ttf"||ext==".otf") return AssetType::Font;
    if (ext==".lua"||ext==".py"||ext==".js") return AssetType::Script;
    return AssetType::Binary;
}

// ──────────────────────────────────────────────────────────────
// Write .atlasb
// ──────────────────────────────────────────────────────────────

ImportResult AssetImporter::WriteAtlasb(const std::string& outPath, AssetType type,
                                         const std::string& name,
                                         const std::vector<uint8_t>& payload) {
    ImportResult res;

    AssetHeader hdr;
    hdr.type = type;
    hdr.size = (uint32_t)payload.size();
    hdr.hash = HashPayload(payload.data(), payload.size());
    // Copy name (null-terminated, truncate if needed)
    size_t nameLen = std::min(name.size(), sizeof(hdr.name) - 1);
    std::copy_n(name.begin(), nameLen, hdr.name);

    std::ofstream out(outPath, std::ios::binary);
    if (!out) {
        res.errorMessage = "Cannot create output: " + outPath;
        return res;
    }
    out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    out.write(reinterpret_cast<const char*>(payload.data()), (std::streamsize)payload.size());
    if (!out.good()) {
        res.errorMessage = "Write failed: " + outPath;
        return res;
    }

    res.success      = true;
    res.outputType   = type;
    res.outputPath   = outPath;
    res.hash         = hdr.hash;
    res.payloadBytes = hdr.size;
    return res;
}

// ──────────────────────────────────────────────────────────────
// Import
// ──────────────────────────────────────────────────────────────

ImportResult AssetImporter::Import(const std::string& filePath, const ImportOptions& options) {
    AssetType type = InferType(filePath);
    switch (type) {
    case AssetType::Mesh:    return ImportMesh(filePath, options);
    case AssetType::Texture: return ImportTexture(filePath, options);
    case AssetType::Audio:   return ImportAudio(filePath, options);
    default: break;
    }

    // Generic binary passthrough
    ImportResult res;
    if (!std::filesystem::exists(filePath)) {
        res.errorMessage = "File not found: " + filePath;
        return res;
    }
    if (!options.overwrite) {
        std::string out = (options.outputDirectory.empty()
            ? std::filesystem::path(filePath).parent_path().string()
            : options.outputDirectory) + "/" + StemName(filePath) + ".atlasb";
        if (std::filesystem::exists(out)) {
            res.success = true; res.outputPath = out; return res;
        }
    }

    std::ifstream in(filePath, std::ios::binary | std::ios::ate);
    if (!in) { res.errorMessage = "Cannot open: " + filePath; return res; }
    auto sz = (size_t)in.tellg(); in.seekg(0);
    std::vector<uint8_t> data(sz);
    in.read(reinterpret_cast<char*>(data.data()), (std::streamsize)sz);

    std::string outDir = options.outputDirectory.empty()
        ? std::filesystem::path(filePath).parent_path().string()
        : options.outputDirectory;
    std::filesystem::create_directories(outDir);
    std::string outPath = outDir + "/" + StemName(filePath) + ".atlasb";
    return WriteAtlasb(outPath, type, StemName(filePath), data);
}

ImportResult AssetImporter::ImportMesh(const std::string& filePath,
                                        const ImportOptions& options) {
    ImportResult res;
    if (!std::filesystem::exists(filePath)) {
        res.errorMessage = "Mesh file not found: " + filePath;
        return res;
    }
    // Read raw mesh data (full Assimp/TinyGLTF integration would go here)
    std::ifstream in(filePath, std::ios::binary | std::ios::ate);
    if (!in) { res.errorMessage = "Cannot open: " + filePath; return res; }
    auto sz = (size_t)in.tellg(); in.seekg(0);
    std::vector<uint8_t> data(sz);
    in.read(reinterpret_cast<char*>(data.data()), (std::streamsize)sz);

    std::string outDir = options.outputDirectory.empty()
        ? std::filesystem::path(filePath).parent_path().string()
        : options.outputDirectory;
    std::filesystem::create_directories(outDir);
    return WriteAtlasb(outDir + "/" + StemName(filePath) + ".atlasb",
                       AssetType::Mesh, StemName(filePath), data);
}

ImportResult AssetImporter::ImportTexture(const std::string& filePath,
                                           const ImportOptions& options) {
    ImportResult res;
    if (!std::filesystem::exists(filePath)) {
        res.errorMessage = "Texture file not found: " + filePath;
        return res;
    }
    std::ifstream in(filePath, std::ios::binary | std::ios::ate);
    if (!in) { res.errorMessage = "Cannot open: " + filePath; return res; }
    auto sz = (size_t)in.tellg(); in.seekg(0);
    std::vector<uint8_t> data(sz);
    in.read(reinterpret_cast<char*>(data.data()), (std::streamsize)sz);

    std::string outDir = options.outputDirectory.empty()
        ? std::filesystem::path(filePath).parent_path().string()
        : options.outputDirectory;
    std::filesystem::create_directories(outDir);
    return WriteAtlasb(outDir + "/" + StemName(filePath) + ".atlasb",
                       AssetType::Texture, StemName(filePath), data);
}

ImportResult AssetImporter::ImportAudio(const std::string& filePath,
                                         const ImportOptions& options) {
    ImportResult res;
    if (!std::filesystem::exists(filePath)) {
        res.errorMessage = "Audio file not found: " + filePath;
        return res;
    }
    std::ifstream in(filePath, std::ios::binary | std::ios::ate);
    if (!in) { res.errorMessage = "Cannot open: " + filePath; return res; }
    auto sz = (size_t)in.tellg(); in.seekg(0);
    std::vector<uint8_t> data(sz);
    in.read(reinterpret_cast<char*>(data.data()), (std::streamsize)sz);

    std::string outDir = options.outputDirectory.empty()
        ? std::filesystem::path(filePath).parent_path().string()
        : options.outputDirectory;
    std::filesystem::create_directories(outDir);
    return WriteAtlasb(outDir + "/" + StemName(filePath) + ".atlasb",
                       AssetType::Audio, StemName(filePath), data);
}

std::vector<ImportResult> AssetImporter::ImportDirectory(const std::string& dirPath,
                                                          const ImportOptions& options,
                                                          bool recursive) {
    std::vector<ImportResult> results;
    if (!std::filesystem::exists(dirPath)) return results;

    auto process = [&](const std::filesystem::directory_entry& entry) {
        if (!entry.is_regular_file()) return;
        std::string ext = LowerExtension(entry.path().string());
        if (IsSupportedMesh(ext) || IsSupportedTexture(ext) || IsSupportedAudio(ext))
            results.push_back(Import(entry.path().string(), options));
    };

    if (recursive) {
        for (const auto& e : std::filesystem::recursive_directory_iterator(dirPath))
            process(e);
    } else {
        for (const auto& e : std::filesystem::directory_iterator(dirPath))
            process(e);
    }
    return results;
}

bool AssetImporter::ReadHeader(const std::string& atlasbPath, AssetHeader& header) {
    std::ifstream f(atlasbPath, std::ios::binary);
    if (!f) return false;
    f.read(reinterpret_cast<char*>(&header), sizeof(header));
    return f.good();
}

} // namespace Tools
