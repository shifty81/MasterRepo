#include "Runtime/SaveGame/SaveGameManager.h"
#include <chrono>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace Runtime {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

struct SaveGameManager::Impl {
    std::string                                    saveDir;
    std::unordered_map<uint32_t, SaveSlotInfo>     slotMeta;
    std::unordered_map<std::string, SaveSlotInfo>  namedMeta;
    uint32_t                                       currentSchemaVersion{1};

    struct MigrationEntry {
        uint32_t from, to;
        std::function<void(SaveData&)> fn;
    };
    std::vector<MigrationEntry> migrations;

    bool     autoSaveEnabled{false};
    float    autoSaveInterval{300.f};
    float    autoSaveAccum{0.f};
    uint32_t autoSaveSlot{0};
    uint64_t playtimeSeconds{0};
    float    playtimeAccumSeconds{0.f};

    std::function<SaveData()>             autoSaveProvider;
    std::function<void(uint32_t)>         onSave;
    std::function<void(uint32_t)>         onLoad;
    std::function<void(const std::string&)> onError;

    std::string SlotPath(uint32_t slot) const {
        return saveDir + "save_slot_" + std::to_string(slot) + ".json";
    }

    std::string NamedPath(const std::string& name) const {
        return saveDir + "save_" + name + ".json";
    }

    SaveData MigrateData(SaveData data, uint32_t fromVer, uint32_t toVer) {
        for (uint32_t v = fromVer; v < toVer; ++v) {
            for (auto& m : migrations)
                if (m.from == v && m.to == v+1) m.fn(data);
        }
        return data;
    }
};

SaveGameManager::SaveGameManager() : m_impl(new Impl()) {}
SaveGameManager::~SaveGameManager() { delete m_impl; }

void SaveGameManager::Init(const std::string& saveDir) {
    m_impl->saveDir = saveDir;
    // Ensure trailing slash
    if (!m_impl->saveDir.empty() && m_impl->saveDir.back() != '/')
        m_impl->saveDir += '/';
}

void SaveGameManager::Shutdown() {}

bool SaveGameManager::SaveSlot(uint32_t slot, const SaveData& data,
                               const std::string& description) {
    std::string path = m_impl->SlotPath(slot);
    std::ofstream f(path);
    if (!f.is_open()) {
        if (m_impl->onError) m_impl->onError("Cannot open: " + path);
        return false;
    }
    f << data;
    SaveSlotInfo& info     = m_impl->slotMeta[slot];
    info.slot              = slot;
    info.description       = description;
    info.timestampMs       = NowMs();
    info.playtimeSeconds   = m_impl->playtimeSeconds;
    info.schemaVersion     = m_impl->currentSchemaVersion;
    info.isEmpty           = false;
    if (m_impl->onSave) m_impl->onSave(slot);
    return true;
}

SaveData SaveGameManager::LoadSlot(uint32_t slot) {
    std::string path = m_impl->SlotPath(slot);
    std::ifstream f(path);
    if (!f.is_open()) return {};
    std::stringstream ss; ss << f.rdbuf();
    SaveData data = ss.str();
    uint32_t fileVer = m_impl->slotMeta.count(slot)
                       ? m_impl->slotMeta[slot].schemaVersion : 1;
    data = m_impl->MigrateData(data, fileVer, m_impl->currentSchemaVersion);
    if (m_impl->onLoad) m_impl->onLoad(slot);
    return data;
}

bool SaveGameManager::DeleteSlot(uint32_t slot) {
    m_impl->slotMeta.erase(slot);
    return std::remove(m_impl->SlotPath(slot).c_str()) == 0;
}

bool SaveGameManager::SlotExists(uint32_t slot) const {
    std::ifstream f(m_impl->SlotPath(slot));
    return f.good();
}

bool SaveGameManager::SaveNamed(const std::string& name, const SaveData& data) {
    std::ofstream f(m_impl->NamedPath(name));
    if (!f.is_open()) return false;
    f << data;
    auto& info = m_impl->namedMeta[name];
    info.name          = name;
    info.timestampMs   = NowMs();
    info.schemaVersion = m_impl->currentSchemaVersion;
    info.isEmpty       = false;
    return true;
}

SaveData SaveGameManager::LoadNamed(const std::string& name) {
    std::ifstream f(m_impl->NamedPath(name));
    if (!f.is_open()) return {};
    std::stringstream ss; ss << f.rdbuf();
    return ss.str();
}

SaveSlotInfo SaveGameManager::GetSlotInfo(uint32_t slot) const {
    auto it = m_impl->slotMeta.find(slot);
    if (it == m_impl->slotMeta.end()) { SaveSlotInfo s; s.slot=slot; return s; }
    return it->second;
}

std::vector<SaveSlotInfo> SaveGameManager::ListSlots() const {
    std::vector<SaveSlotInfo> out;
    for (auto& [k, v] : m_impl->slotMeta) out.push_back(v);
    return out;
}

std::vector<SaveSlotInfo> SaveGameManager::ListNamedSaves() const {
    std::vector<SaveSlotInfo> out;
    for (auto& [k, v] : m_impl->namedMeta) out.push_back(v);
    return out;
}

void SaveGameManager::SetCurrentSchemaVersion(uint32_t v) {
    m_impl->currentSchemaVersion = v;
}

void SaveGameManager::SetMigration(uint32_t from, uint32_t to,
                                   std::function<void(SaveData&)> fn) {
    m_impl->migrations.push_back({from, to, std::move(fn)});
}

void SaveGameManager::EnableAutoSave(float interval, uint32_t slot) {
    m_impl->autoSaveEnabled  = true;
    m_impl->autoSaveInterval = interval;
    m_impl->autoSaveSlot     = slot;
    m_impl->autoSaveAccum    = 0.f;
}

void SaveGameManager::DisableAutoSave() { m_impl->autoSaveEnabled = false; }

void SaveGameManager::Tick(float dt) {
    if (!m_impl->autoSaveEnabled || !m_impl->autoSaveProvider) return;
    m_impl->autoSaveAccum         += dt;
    m_impl->playtimeAccumSeconds  += dt;
    // Accumulate whole seconds into playtimeSeconds to avoid float truncation
    if (m_impl->playtimeAccumSeconds >= 1.0f) {
        uint64_t whole = static_cast<uint64_t>(m_impl->playtimeAccumSeconds);
        m_impl->playtimeSeconds      += whole;
        m_impl->playtimeAccumSeconds -= static_cast<float>(whole);
    }
    if (m_impl->autoSaveAccum >= m_impl->autoSaveInterval) {
        m_impl->autoSaveAccum = 0.f;
        SaveSlot(m_impl->autoSaveSlot, m_impl->autoSaveProvider(), "Auto-save");
    }
}

void SaveGameManager::SetAutoSaveProvider(std::function<SaveData()> p) {
    m_impl->autoSaveProvider = std::move(p);
}

void SaveGameManager::OnSaveComplete(std::function<void(uint32_t)> cb) {
    m_impl->onSave = std::move(cb);
}

void SaveGameManager::OnLoadComplete(std::function<void(uint32_t)> cb) {
    m_impl->onLoad = std::move(cb);
}

void SaveGameManager::OnSaveError(std::function<void(const std::string&)> cb) {
    m_impl->onError = std::move(cb);
}

void SaveGameManager::SetPlaytimeSeconds(uint64_t s) { m_impl->playtimeSeconds = s; }
uint64_t SaveGameManager::PlaytimeSeconds() const { return m_impl->playtimeSeconds; }

} // namespace Runtime
