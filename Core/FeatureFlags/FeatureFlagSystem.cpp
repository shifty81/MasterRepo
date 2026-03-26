#include "Core/FeatureFlags/FeatureFlagSystem.h"
#include <cstdlib>
#include <fstream>
#include <functional>
#include <sstream>

namespace Core {

// ── Deterministic user-bucket hash (0–99) ────────────────────────────────────

static uint32_t UserBucket(const std::string& userId, const std::string& flagId) {
    uint32_t h = 2166136261u;
    for (char c : userId)  h = (h ^ c) * 16777619u;
    for (char c : flagId)  h = (h ^ c) * 16777619u;
    return h % 100;
}

struct FlagState {
    FeatureFlag flag;
    bool        overrideBool{false};
    std::string overrideVariant;
    bool        hasOverride{false};
};

struct FeatureFlagSystem::Impl {
    std::unordered_map<std::string, FlagState>  flags;
    std::string                                 userId;
    std::string                                 configPath;
    std::function<void(const std::string&, bool)> onChange;
};

FeatureFlagSystem::FeatureFlagSystem() : m_impl(new Impl()) {}
FeatureFlagSystem::~FeatureFlagSystem() { delete m_impl; }

void FeatureFlagSystem::Init(const std::string& path) {
    m_impl->configPath = path;
    Load();
}

void FeatureFlagSystem::Shutdown() { Save(); }

void FeatureFlagSystem::SetUserId(const std::string& id) { m_impl->userId = id; }
std::string FeatureFlagSystem::GetUserId() const { return m_impl->userId; }

bool FeatureFlagSystem::IsEnabled(const std::string& id) const {
    auto it = m_impl->flags.find(id);
    if (it == m_impl->flags.end()) return false;
    auto& fs = it->second;
    if (fs.hasOverride) return fs.overrideBool;
    switch (fs.flag.type) {
        case FlagType::Boolean:    return fs.flag.defaultBool;
        case FlagType::Percentage: return UserBucket(m_impl->userId, id) < (uint32_t)fs.flag.rolloutPct;
        case FlagType::Variant:    return !fs.flag.defaultVariant.empty();
    }
    return false;
}

std::string FeatureFlagSystem::GetVariant(const std::string& id,
                                          const std::string& def) const {
    auto it = m_impl->flags.find(id);
    if (it == m_impl->flags.end()) return def;
    auto& fs = it->second;
    if (fs.hasOverride && !fs.overrideVariant.empty()) return fs.overrideVariant;
    if (!fs.flag.defaultVariant.empty()) return fs.flag.defaultVariant;
    // Bucket-select variant
    if (!fs.flag.variants.empty()) {
        uint32_t bucket = UserBucket(m_impl->userId, id);
        uint32_t sz     = static_cast<uint32_t>(fs.flag.variants.size());
        return fs.flag.variants[bucket % sz];
    }
    return def;
}

float FeatureFlagSystem::GetRolloutPct(const std::string& id) const {
    auto it = m_impl->flags.find(id);
    return it != m_impl->flags.end() ? it->second.flag.rolloutPct : 0.f;
}

void FeatureFlagSystem::DefineFlag(const FeatureFlag& flag) {
    m_impl->flags[flag.id].flag = flag;
}

void FeatureFlagSystem::DefineBoolFlag(const std::string& id, bool def,
                                       const std::string& desc) {
    FeatureFlag f; f.id=id; f.type=FlagType::Boolean; f.defaultBool=def; f.description=desc;
    DefineFlag(f);
}

void FeatureFlagSystem::DefinePercentFlag(const std::string& id, float pct,
                                          const std::string& desc) {
    FeatureFlag f; f.id=id; f.type=FlagType::Percentage; f.rolloutPct=pct; f.description=desc;
    DefineFlag(f);
}

void FeatureFlagSystem::DefineVariantFlag(const std::string& id,
    const std::vector<std::string>& variants,
    const std::string& def, const std::string& desc) {
    FeatureFlag f; f.id=id; f.type=FlagType::Variant;
    f.variants=variants; f.defaultVariant=def; f.description=desc;
    DefineFlag(f);
}

bool FeatureFlagSystem::HasFlag(const std::string& id) const {
    return m_impl->flags.count(id) > 0;
}

FeatureFlag FeatureFlagSystem::GetFlag(const std::string& id) const {
    auto it = m_impl->flags.find(id);
    return it != m_impl->flags.end() ? it->second.flag : FeatureFlag{};
}

std::vector<FeatureFlag> FeatureFlagSystem::AllFlags() const {
    std::vector<FeatureFlag> out;
    for (auto& [k, v] : m_impl->flags) out.push_back(v.flag);
    return out;
}

void FeatureFlagSystem::SetOverrideBool(const std::string& id, bool val) {
    auto& fs = m_impl->flags[id];
    fs.hasOverride  = true;
    fs.overrideBool = val;
    if (m_impl->onChange) m_impl->onChange(id, val);
}

void FeatureFlagSystem::SetOverrideVariant(const std::string& id,
                                           const std::string& var) {
    auto& fs = m_impl->flags[id];
    fs.hasOverride     = true;
    fs.overrideVariant = var;
}

void FeatureFlagSystem::ClearOverride(const std::string& id) {
    auto it = m_impl->flags.find(id);
    if (it != m_impl->flags.end()) { it->second.hasOverride = false; }
}

void FeatureFlagSystem::ClearAllOverrides() {
    for (auto& [k, fs] : m_impl->flags) fs.hasOverride = false;
}

void FeatureFlagSystem::Save() const {
    if (m_impl->configPath.empty()) return;
    std::ofstream f(m_impl->configPath);
    if (!f.is_open()) return;
    f << "{\"flags\":[";
    bool first = true;
    for (auto& [id, fs] : m_impl->flags) {
        if (!first) f << ","; first = false;
        f << "{\"id\":\"" << id
          << "\",\"type\":" << static_cast<int>(fs.flag.type)
          << ",\"default\":" << (fs.flag.defaultBool ? "true" : "false") << "}";
    }
    f << "]}\n";
}

void FeatureFlagSystem::Load() { /* reads configPath JSON — stub */ }

void FeatureFlagSystem::OnFlagChanged(
    std::function<void(const std::string&, bool)> cb) {
    m_impl->onChange = std::move(cb);
}

} // namespace Core
