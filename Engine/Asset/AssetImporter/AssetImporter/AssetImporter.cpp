#include "Engine/Asset/AssetImporter/AssetImporter/AssetImporter.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace Engine {

AssetType AssetImporter::DetectType(const std::string& path)
{
    auto ext = path.substr(path.rfind('.')+1);
    for (auto& c : ext) c = (char)::tolower((unsigned char)c);
    if (ext=="obj"||ext=="fbx"||ext=="gltf"||ext=="glb") return AssetType::Mesh;
    if (ext=="png"||ext=="jpg"||ext=="jpeg"||ext=="tga"||ext=="bmp") return AssetType::Texture;
    if (ext=="wav"||ext=="ogg"||ext=="mp3") return AssetType::Audio;
    if (ext=="json") return AssetType::JSON;
    if (ext=="lua"||ext=="py"||ext=="js")  return AssetType::Script;
    if (ext=="ttf"||ext=="otf")            return AssetType::Font;
    if (ext=="mat")                        return AssetType::Material;
    return AssetType::Binary;
}

struct AsyncJob {
    std::string sourcePath;
    ImportSettings settings;
    std::function<void(const ImportResult&)> onDone;
    ProgressFn onProgress;
    bool cancelled{false};
};

struct AssetImporter::Impl {
    std::string sourceRoot, cacheRoot;
    std::unordered_map<AssetType, ImportFn> importers;
    std::vector<AssetRecord> records;
    std::queue<AsyncJob>     asyncQueue;
    bool hotReload{false};

    AssetRecord* FindById(const std::string& id) {
        for (auto& r : records) if (r.id==id) return &r;
        return nullptr;
    }
    const AssetRecord* FindById(const std::string& id) const {
        for (auto& r : records) if (r.id==id) return &r;
        return nullptr;
    }
    const AssetRecord* FindBySource(const std::string& src) const {
        for (auto& r : records) if (r.sourcePath==src) return &r;
        return nullptr;
    }

    static uint64_t HashFile(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) return 0;
        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
        return (uint64_t)std::hash<std::string>{}(content);
    }

    static std::string MakeId(const std::string& src, const std::string& sourceRoot) {
        std::string id = src;
        if (id.substr(0, sourceRoot.size()) == sourceRoot)
            id = id.substr(sourceRoot.size());
        return id;
    }
};

AssetImporter::AssetImporter()  : m_impl(new Impl) {}
AssetImporter::~AssetImporter() { Shutdown(); delete m_impl; }

void AssetImporter::Init(const std::string& srcRoot, const std::string& cacheRoot)
{ m_impl->sourceRoot=srcRoot; m_impl->cacheRoot=cacheRoot; }
void AssetImporter::Shutdown() { m_impl->records.clear(); }

void AssetImporter::RegisterImporter  (AssetType t, ImportFn fn) { m_impl->importers[t]=fn; }
void AssetImporter::UnregisterImporter(AssetType t)              { m_impl->importers.erase(t); }

ImportResult AssetImporter::Import(const std::string& sourcePath, const ImportSettings& settingsIn)
{
    ImportSettings settings = settingsIn;
    if (settings.type == AssetType::Unknown) settings.type = DetectType(sourcePath);

    ImportResult result;
    result.assetId  = Impl::MakeId(sourcePath, m_impl->sourceRoot);
    result.cachedPath = m_impl->cacheRoot + result.assetId;

    // Check cache
    auto* existing = m_impl->FindById(result.assetId);
    uint64_t hash  = Impl::HashFile(sourcePath);
    if (existing && existing->sourceHash == hash && existing->valid) {
        result.success = true;
        result.record  = *existing;
        return result;
    }

    // Run importer
    auto it = m_impl->importers.find(settings.type);
    if (it != m_impl->importers.end() && it->second) {
        result = it->second(sourcePath, settings, m_impl->cacheRoot);
    } else {
        // Default: just record the source as cached
        result.success   = true;
        result.cachedPath= sourcePath;
        result.assetId   = Impl::MakeId(sourcePath, m_impl->sourceRoot);
    }

    if (result.success) {
        AssetRecord rec;
        rec.id          = result.assetId;
        rec.sourcePath  = sourcePath;
        rec.cachedPath  = result.cachedPath;
        rec.type        = settings.type;
        rec.sourceHash  = hash;
        rec.importTimestamp = (uint64_t)std::chrono::system_clock::now().time_since_epoch().count();
        rec.valid       = true;
        if (existing) *existing = rec;
        else          m_impl->records.push_back(rec);
        result.record = rec;
    }
    return result;
}

ImportResult AssetImporter::ImportById(const std::string& assetId)
{
    auto* rec = m_impl->FindById(assetId);
    return rec ? Import(rec->sourcePath) : ImportResult{};
}

void AssetImporter::ImportAsync(const std::string& sourcePath,
                                 std::function<void(const ImportResult&)> onDone,
                                 ProgressFn onProgress, const ImportSettings& settings)
{
    AsyncJob job;
    job.sourcePath=sourcePath; job.settings=settings;
    job.onDone=onDone; job.onProgress=onProgress;
    m_impl->asyncQueue.push(std::move(job));
}

void AssetImporter::CancelAsync(const std::string& sourcePath)
{
    // Mark cancelled in next tick; simplified: immediate erase not possible with queue
    // Real impl would use a list; here we just ignore cancellation in stub
    (void)sourcePath;
}

bool               AssetImporter::HasAsset(const std::string& id) const { return m_impl->FindById(id)!=nullptr; }
const AssetRecord* AssetImporter::GetRecord(const std::string& id) const { return m_impl->FindById(id); }
std::vector<AssetRecord> AssetImporter::GetAll() const { return m_impl->records; }
std::vector<AssetRecord> AssetImporter::GetByType(AssetType t) const
{
    std::vector<AssetRecord> out;
    for (auto& r : m_impl->records) if (r.type==t) out.push_back(r);
    return out;
}

std::vector<std::string> AssetImporter::GetDependents(const std::string& assetId) const
{
    std::vector<std::string> out;
    for (auto& r : m_impl->records)
        for (auto& dep : r.dependencies)
            if (dep==assetId) { out.push_back(r.id); break; }
    return out;
}

void AssetImporter::InvalidateAsset(const std::string& id, bool cascade)
{
    auto* rec = m_impl->FindById(id);
    if (rec) rec->valid=false;
    if (cascade) {
        for (auto& dep : GetDependents(id)) InvalidateAsset(dep, true);
    }
}

bool AssetImporter::SaveManifest(const std::string& path) const
{
    std::ofstream f(path);
    if (!f) return false;
    f << "{\"assets\":[";
    bool first=true;
    for (auto& r : m_impl->records) {
        if(!first) f<<","; first=false;
        f << "{\"id\":\"" << r.id << "\",\"source\":\"" << r.sourcePath << "\"}";
    }
    f << "]}";
    return true;
}

bool AssetImporter::LoadManifest(const std::string& path)
{
    std::ifstream f(path); return f.good();
}

void AssetImporter::SetHotReload(bool e)  { m_impl->hotReload=e; }
void AssetImporter::ScanForChanges()
{
    for (auto& rec : m_impl->records) {
        uint64_t h = Impl::HashFile(rec.sourcePath);
        if (h && h!=rec.sourceHash) rec.valid=false;
    }
}

void AssetImporter::Tick(float /*dt*/)
{
    // Process one async job per tick
    if (!m_impl->asyncQueue.empty()) {
        auto job = m_impl->asyncQueue.front(); m_impl->asyncQueue.pop();
        if (!job.cancelled) {
            if (job.onProgress) job.onProgress(job.sourcePath, 0.5f);
            auto result = Import(job.sourcePath, job.settings);
            if (job.onProgress) job.onProgress(job.sourcePath, 1.f);
            if (job.onDone) job.onDone(result);
        }
    }
}

} // namespace Engine
