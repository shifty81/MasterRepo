#include "Plugins/Marketplace/PluginMarketplace.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;
namespace Plugins {

// ─────────────────────────────────────────────────────────────────────────────
// Minimal JSON helpers (no external deps)
// ─────────────────────────────────────────────────────────────────────────────

static std::string JsonStr(const std::string& key, const std::string& val)
{
    return "\"" + key + "\": \"" + val + "\"";
}

static std::string ReadFile(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs) return {};
    return {std::istreambuf_iterator<char>(ifs),
            std::istreambuf_iterator<char>()};
}

// Very minimal JSON value extractor (handles simple string/integer fields).
static std::string JsonExtract(const std::string& json, const std::string& key)
{
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return {};
    pos += needle.size();
    // skip whitespace and ':'
    while (pos < json.size() && (json[pos]==' '||json[pos]=='\t'||json[pos]==':')) ++pos;
    if (pos >= json.size()) return {};
    if (json[pos] == '"') {
        // string value
        ++pos;
        auto end = json.find('"', pos);
        if (end == std::string::npos) return {};
        return json.substr(pos, end - pos);
    }
    // number / bool
    auto end = json.find_first_of(",}\n", pos);
    if (end == std::string::npos) end = json.size();
    std::string v = json.substr(pos, end - pos);
    // trim
    v.erase(0, v.find_first_not_of(" \t"));
    v.erase(v.find_last_not_of(" \t\r") + 1);
    return v;
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

PluginMarketplace::PluginMarketplace(MarketplaceConfig cfg)
    : m_cfg(std::move(cfg))
{
    LoadCatalogue();
}

PluginMarketplace::~PluginMarketplace() = default;

// ─────────────────────────────────────────────────────────────────────────────
// Catalogue refresh
// ─────────────────────────────────────────────────────────────────────────────

void PluginMarketplace::RefreshCatalogue()
{
    // Scan local source directories
    for (const auto& dir : m_cfg.localSourceDirs)
        ScanLocalDir(dir);

    // Remote fetch stub — requires network which is offline-first
    // TODO: fetch m_cfg.remoteIndexUrl if non-empty

    SaveCatalogue();
    NotifyChanged();
}

bool PluginMarketplace::ScanLocalDir(const std::string& dir)
{
    if (!fs::exists(dir)) return false;

    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().filename() != "manifest.json") continue;

        PluginPackEntry pack;
        if (ParsePackManifest(entry.path().string(), pack)) {
            // Update existing entry or append
            auto it = std::find_if(m_packs.begin(), m_packs.end(),
                                   [&](const PluginPackEntry& p){ return p.id == pack.id; });
            if (it != m_packs.end()) {
                // Preserve install state
                pack.state         = it->state;
                pack.installedPath = it->installedPath;
                *it = std::move(pack);
            } else {
                m_packs.push_back(std::move(pack));
            }
        }
    }
    return true;
}

bool PluginMarketplace::ParsePackManifest(const std::string& jsonPath,
                                           PluginPackEntry& out)
{
    std::string json = ReadFile(jsonPath);
    if (json.empty()) return false;

    out.id           = JsonExtract(json, "id");
    out.displayName  = JsonExtract(json, "name");
    out.description  = JsonExtract(json, "description");
    out.author       = JsonExtract(json, "author");
    out.version      = JsonExtract(json, "version");
    out.category     = JsonExtract(json, "category");
    out.licenseType  = JsonExtract(json, "license");
    out.sourceUrl    = fs::path(jsonPath).parent_path().string();

    return !out.id.empty();
}

// ─────────────────────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────────────────────

std::vector<PluginPackEntry> PluginMarketplace::GetAllPacks() const
{
    return m_packs;
}

std::vector<PluginPackEntry> PluginMarketplace::GetPacksByState(PackState state) const
{
    std::vector<PluginPackEntry> result;
    for (const auto& p : m_packs)
        if (p.state == state) result.push_back(p);
    return result;
}

std::vector<PluginPackEntry> PluginMarketplace::GetPacksByCategory(const std::string& cat) const
{
    std::vector<PluginPackEntry> result;
    std::string lcat = cat;
    for (char& c : lcat) c = (char)std::tolower((unsigned char)c);

    for (const auto& p : m_packs) {
        std::string pcat = p.category;
        for (char& c : pcat) c = (char)std::tolower((unsigned char)c);
        if (pcat == lcat) result.push_back(p);
    }
    return result;
}

const PluginPackEntry* PluginMarketplace::FindPack(const std::string& id) const
{
    auto it = std::find_if(m_packs.begin(), m_packs.end(),
                           [&](const PluginPackEntry& p){ return p.id == id; });
    return (it != m_packs.end()) ? &(*it) : nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// Install / Uninstall / Update
// ─────────────────────────────────────────────────────────────────────────────

PluginMarketplace::InstallResult
PluginMarketplace::Install(const std::string& packId,
                            std::function<void(float)> progressCb)
{
    InstallResult result;
    result.packId = packId;

    auto it = std::find_if(m_packs.begin(), m_packs.end(),
                           [&](const PluginPackEntry& p){ return p.id == packId; });
    if (it == m_packs.end()) {
        result.errorMessage = "Pack not found: " + packId;
        return result;
    }

    if (progressCb) progressCb(0.0f);

    // Create install directory
    std::string dest = m_cfg.installDir + "/" + packId;
    try {
        fs::create_directories(dest);

        // Copy source files
        if (!it->sourceUrl.empty() && fs::exists(it->sourceUrl)) {
            int n = 0;
            std::vector<fs::path> files;
            for (const auto& e : fs::recursive_directory_iterator(it->sourceUrl))
                if (e.is_regular_file()) files.push_back(e.path());

            for (size_t i = 0; i < files.size(); ++i) {
                fs::path rel = fs::relative(files[i], it->sourceUrl);
                fs::path dst = fs::path(dest) / rel;
                fs::create_directories(dst.parent_path());
                fs::copy_file(files[i], dst, fs::copy_options::overwrite_existing);
                if (progressCb)
                    progressCb(static_cast<float>(i + 1) / static_cast<float>(files.size()));
            }
        }

        it->state         = PackState::Installed;
        it->installedPath = dest;
        result.success    = true;
        result.installedPath = dest;

        SaveCatalogue();
        NotifyChanged();
    } catch (const std::exception& e) {
        it->state        = PackState::Error;
        it->errorMessage = e.what();
        result.errorMessage = e.what();
    }

    if (progressCb) progressCb(1.0f);
    return result;
}

bool PluginMarketplace::Uninstall(const std::string& packId)
{
    auto it = std::find_if(m_packs.begin(), m_packs.end(),
                           [&](const PluginPackEntry& p){ return p.id == packId; });
    if (it == m_packs.end()) return false;
    if (it->state != PackState::Installed && it->state != PackState::Disabled) return false;

    try {
        if (!it->installedPath.empty() && fs::exists(it->installedPath))
            fs::remove_all(it->installedPath);
        it->state         = PackState::Available;
        it->installedPath = {};
        SaveCatalogue();
        NotifyChanged();
        return true;
    } catch (...) {
        return false;
    }
}

PluginMarketplace::InstallResult
PluginMarketplace::Update(const std::string& packId,
                           std::function<void(float)> progressCb)
{
    Uninstall(packId);
    return Install(packId, std::move(progressCb));
}

// ─────────────────────────────────────────────────────────────────────────────
// Enable / Disable
// ─────────────────────────────────────────────────────────────────────────────

bool PluginMarketplace::Enable(const std::string& packId)
{
    auto it = std::find_if(m_packs.begin(), m_packs.end(),
                           [&](const PluginPackEntry& p){ return p.id == packId; });
    if (it == m_packs.end()) return false;
    if (it->state == PackState::Disabled) {
        it->state = PackState::Installed;
        SaveCatalogue();
        NotifyChanged();
        return true;
    }
    return false;
}

bool PluginMarketplace::Disable(const std::string& packId)
{
    auto it = std::find_if(m_packs.begin(), m_packs.end(),
                           [&](const PluginPackEntry& p){ return p.id == packId; });
    if (it == m_packs.end()) return false;
    if (it->state == PackState::Installed) {
        it->state = PackState::Disabled;
        SaveCatalogue();
        NotifyChanged();
        return true;
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Export
// ─────────────────────────────────────────────────────────────────────────────

std::string PluginMarketplace::Export(const std::string& packId,
                                       const std::string& outputDir)
{
    const PluginPackEntry* p = FindPack(packId);
    if (!p || p->state != PackState::Installed) return {};

    try {
        fs::create_directories(outputDir);
        std::string archiveName = packId + "-" + p->version + ".atlasPack";
        std::string archivePath = (fs::path(outputDir) / archiveName).string();

        // Write a simple manifest JSON as the archive (full archive impl
        // would use zlib/miniz for compression — this is the scaffold)
        std::ofstream ofs(archivePath);
        if (!ofs) return {};
        ofs << "{\n"
            << "  " << JsonStr("id",      p->id)          << ",\n"
            << "  " << JsonStr("name",    p->displayName) << ",\n"
            << "  " << JsonStr("version", p->version)     << ",\n"
            << "  " << JsonStr("author",  p->author)      << ",\n"
            << "  " << JsonStr("source",  p->installedPath) << "\n"
            << "}\n";

        return archivePath;
    } catch (...) {
        return {};
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Persistence
// ─────────────────────────────────────────────────────────────────────────────

static std::string StateToStr(PackState s)
{
    switch (s) {
        case PackState::Available:       return "available";
        case PackState::Installed:       return "installed";
        case PackState::UpdateAvailable: return "update_available";
        case PackState::Disabled:        return "disabled";
        case PackState::Error:           return "error";
    }
    return "available";
}

static PackState StrToState(const std::string& s)
{
    if (s == "installed")        return PackState::Installed;
    if (s == "update_available") return PackState::UpdateAvailable;
    if (s == "disabled")         return PackState::Disabled;
    if (s == "error")            return PackState::Error;
    return PackState::Available;
}

bool PluginMarketplace::SaveCatalogue() const
{
    try {
        fs::create_directories(fs::path(m_cfg.catalogueFile).parent_path());
        std::ofstream ofs(m_cfg.catalogueFile);
        if (!ofs) return false;

        ofs << "{\n  \"packs\": [\n";
        for (size_t i = 0; i < m_packs.size(); ++i) {
            const auto& p = m_packs[i];
            ofs << "    {\n"
                << "      " << JsonStr("id",            p.id)           << ",\n"
                << "      " << JsonStr("name",          p.displayName)  << ",\n"
                << "      " << JsonStr("version",       p.version)      << ",\n"
                << "      " << JsonStr("author",        p.author)       << ",\n"
                << "      " << JsonStr("category",      p.category)     << ",\n"
                << "      " << JsonStr("state",         StateToStr(p.state)) << ",\n"
                << "      " << JsonStr("installedPath", p.installedPath) << "\n"
                << "    }" << (i + 1 < m_packs.size() ? "," : "") << "\n";
        }
        ofs << "  ]\n}\n";
        return true;
    } catch (...) {
        return false;
    }
}

bool PluginMarketplace::LoadCatalogue()
{
    std::string json = ReadFile(m_cfg.catalogueFile);
    if (json.empty()) return false;

    // Very minimal array parser — finds '{' blocks inside "packs": [...]
    auto start = json.find("\"packs\"");
    if (start == std::string::npos) return false;
    start = json.find('[', start);
    if (start == std::string::npos) return false;

    size_t pos = start + 1;
    while (pos < json.size()) {
        auto ob = json.find('{', pos);
        if (ob == std::string::npos) break;
        auto cb = json.find('}', ob);
        if (cb == std::string::npos) break;

        std::string block = json.substr(ob, cb - ob + 1);
        PluginPackEntry p;
        p.id            = JsonExtract(block, "id");
        p.displayName   = JsonExtract(block, "name");
        p.version       = JsonExtract(block, "version");
        p.author        = JsonExtract(block, "author");
        p.category      = JsonExtract(block, "category");
        p.state         = StrToState(JsonExtract(block, "state"));
        p.installedPath = JsonExtract(block, "installedPath");

        if (!p.id.empty())
            m_packs.push_back(std::move(p));

        pos = cb + 1;
    }
    return !m_packs.empty();
}

// ─────────────────────────────────────────────────────────────────────────────
// Callbacks
// ─────────────────────────────────────────────────────────────────────────────

void PluginMarketplace::SetCatalogueChangedCallback(CatalogueChangedFn cb)
{
    m_onChanged = std::move(cb);
}

void PluginMarketplace::NotifyChanged()
{
    if (m_onChanged) m_onChanged();
}

} // namespace Plugins
