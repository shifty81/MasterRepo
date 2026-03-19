#include "AssetCooker.h"
#include <filesystem>
#include <fstream>

namespace atlas::production {

void AssetCooker::SetOutputDir(const std::string& dir) {
    m_outputDir = dir;
}

const std::string& AssetCooker::OutputDir() const {
    return m_outputDir;
}

void AssetCooker::SetStripEditorData(bool strip) {
    m_stripEditorData = strip;
}

bool AssetCooker::StripEditorData() const {
    return m_stripEditorData;
}

CookResult AssetCooker::CookAsset(const std::string& sourceId, const std::string& sourcePath) {
    CookEntry entry;
    entry.sourceId = sourceId;
    entry.sourcePath = sourcePath;

    // Check source exists
    if (!std::filesystem::exists(sourcePath)) {
        entry.result = CookResult::SourceNotFound;
        entry.outputPath = "";
        m_cookLog.push_back(std::move(entry));
        return CookResult::SourceNotFound;
    }

    // Determine output path
    std::filesystem::path outDir(m_outputDir);
    std::filesystem::path srcFile(sourcePath);
    std::string outFilename = sourceId + ".atlasb";
    std::filesystem::path outPath = outDir / outFilename;
    entry.outputPath = outPath.string();

    // Create output directory if needed
    std::filesystem::create_directories(outDir);

    // Read source file
    std::ifstream inFile(sourcePath, std::ios::binary);
    if (!inFile.is_open()) {
        entry.result = CookResult::CompileError;
        m_cookLog.push_back(std::move(entry));
        return CookResult::CompileError;
    }

    std::vector<uint8_t> sourceData(
        (std::istreambuf_iterator<char>(inFile)),
        std::istreambuf_iterator<char>()
    );
    inFile.close();

    // Write cooked asset (binary format with header)
    std::ofstream outFile(outPath.string(), std::ios::binary);
    if (!outFile.is_open()) {
        entry.result = CookResult::WriteError;
        m_cookLog.push_back(std::move(entry));
        return CookResult::WriteError;
    }

    // Write simple header: magic(4) + version(4) + flags(4) + dataSize(4)
    uint32_t magic = 0x434F4F4B; // "COOK"
    uint32_t version = 1;
    uint32_t flags = m_stripEditorData ? 1 : 0;
    uint32_t dataSize = static_cast<uint32_t>(sourceData.size());

    outFile.write(reinterpret_cast<const char*>(&magic), 4);
    outFile.write(reinterpret_cast<const char*>(&version), 4);
    outFile.write(reinterpret_cast<const char*>(&flags), 4);
    outFile.write(reinterpret_cast<const char*>(&dataSize), 4);
    outFile.write(reinterpret_cast<const char*>(sourceData.data()), sourceData.size());
    outFile.close();

    entry.result = CookResult::Success;
    m_cookLog.push_back(std::move(entry));
    return CookResult::Success;
}

CookStats AssetCooker::CookAll(const std::string& sourceDir) {
    CookStats stats;

    if (!std::filesystem::exists(sourceDir)) {
        return stats;
    }

    // Count total assets first
    std::vector<std::filesystem::directory_entry> assetFiles;
    for (const auto& p : std::filesystem::recursive_directory_iterator(sourceDir)) {
        if (p.path().extension() == ".atlas" || p.path().extension() == ".atlasb") {
            assetFiles.push_back(p);
        }
    }

    stats.totalAssets = static_cast<uint32_t>(assetFiles.size());

    for (uint32_t i = 0; i < assetFiles.size(); ++i) {
        const auto& p = assetFiles[i];
        std::string id = p.path().stem().string();

        if (m_progressCb) {
            m_progressCb(id, i + 1, stats.totalAssets);
        }

        CookResult result = CookAsset(id, p.path().string());
        switch (result) {
            case CookResult::Success:
                stats.cookedAssets++;
                break;
            case CookResult::SourceNotFound:
                stats.skippedAssets++;
                break;
            default:
                stats.failedAssets++;
                break;
        }
    }

    return stats;
}

void AssetCooker::SetProgressCallback(ProgressCallback cb) {
    m_progressCb = std::move(cb);
}

const std::vector<CookEntry>& AssetCooker::CookLog() const {
    return m_cookLog;
}

void AssetCooker::ClearLog() {
    m_cookLog.clear();
}

}
