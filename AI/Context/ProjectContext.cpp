#include "AI/Context/ProjectContext.h"
#include <algorithm>

namespace AI {

ProjectContext::ProjectContext() = default;

void     ProjectContext::SetGameType(GameType type)            { m_gameType = type; }
GameType ProjectContext::GetGameType() const                   { return m_gameType; }
void     ProjectContext::DetectGameType()                      { /* stub */ }

void              ProjectContext::SetProjectName(const std::string& name) { m_projectName = name; }
const std::string& ProjectContext::GetProjectName() const                 { return m_projectName; }
void              ProjectContext::SetProjectRoot(const std::string& root) { m_projectRoot = root; }
const std::string& ProjectContext::GetProjectRoot() const                 { return m_projectRoot; }

void ProjectContext::ScanAssets() { /* stub */ }

void ProjectContext::AddAsset(const std::string& id, AssetCategory category) {
    if (m_inventory.assetIds.insert(id).second) {
        m_inventory.categoryCounts[category]++;
        m_inventory.totalAssetCount++;
    }
    m_assetCategories[id] = category;
}

void ProjectContext::RemoveAsset(const std::string& id) {
    auto it = m_assetCategories.find(id);
    if (it != m_assetCategories.end()) {
        auto& cnt = m_inventory.categoryCounts[it->second];
        if (cnt > 0) --cnt;
        m_inventory.assetIds.erase(id);
        if (m_inventory.totalAssetCount > 0) --m_inventory.totalAssetCount;
        m_assetCategories.erase(it);
    }
}

bool     ProjectContext::HasAsset(const std::string& id) const      { return m_inventory.assetIds.count(id) > 0; }
bool     ProjectContext::HasAssetCategory(AssetCategory cat) const  {
    auto it = m_inventory.categoryCounts.find(cat);
    return it != m_inventory.categoryCounts.end() && it->second > 0;
}
uint32_t ProjectContext::GetAssetCount(AssetCategory cat) const {
    auto it = m_inventory.categoryCounts.find(cat);
    return it != m_inventory.categoryCounts.end() ? it->second : 0;
}
const AssetInventory& ProjectContext::GetInventory() const { return m_inventory; }

void ProjectContext::AddRequirement(const AssetRequirement& req) { m_requirements.push_back(req); }
void ProjectContext::ClearRequirements() { m_requirements.clear(); }
void ProjectContext::AnalyzeRequirements() {
    auto defaults = GenerateDefaultRequirements(m_gameType);
    m_requirements.insert(m_requirements.end(), defaults.begin(), defaults.end());
    UpdateRequirementsFulfillment();
}
const std::vector<AssetRequirement>& ProjectContext::GetRequirements() const { return m_requirements; }
std::vector<AssetRequirement> ProjectContext::GetUnfulfilledRequirements() const {
    std::vector<AssetRequirement> out;
    for (const auto& r : m_requirements) if (!r.fulfilled) out.push_back(r);
    return out;
}

std::vector<AssetSuggestion> ProjectContext::GenerateSuggestions() const {
    std::vector<AssetSuggestion> suggestions;
    for (const auto& req : m_requirements) {
        if (!req.fulfilled) {
            AssetSuggestion s;
            s.category = req.category;
            s.name = req.name;
            s.description = req.description;
            s.source = "procedural";
            s.confidence = 0.8;
            s.priority = req.priority;
            suggestions.push_back(s);
        }
    }
    return suggestions;
}

std::vector<AssetSuggestion> ProjectContext::GetPrioritySuggestions(uint32_t maxCount) const {
    auto suggestions = GenerateSuggestions();
    std::sort(suggestions.begin(), suggestions.end(),
        [](const AssetSuggestion& a, const AssetSuggestion& b){ return a.priority > b.priority; });
    if (suggestions.size() > maxCount) suggestions.resize(maxCount);
    return suggestions;
}

void ProjectContext::Clear() {
    m_gameType = GameType::Unknown;
    m_projectName.clear();
    m_projectRoot.clear();
    m_inventory = AssetInventory{};
    m_requirements.clear();
    m_assetCategories.clear();
}

std::string ProjectContext::ToJson() const { return "{}"; }
bool ProjectContext::FromJson(const std::string&) { return false; }

const char* ProjectContext::GameTypeToString(GameType type) {
    switch (type) {
        case GameType::RPG:        return "RPG";
        case GameType::FPS:        return "FPS";
        case GameType::TPS:        return "TPS";
        case GameType::Strategy:   return "Strategy";
        case GameType::Platformer: return "Platformer";
        case GameType::Puzzle:     return "Puzzle";
        case GameType::Simulation: return "Simulation";
        case GameType::Racing:     return "Racing";
        case GameType::Adventure:  return "Adventure";
        case GameType::Survival:   return "Survival";
        case GameType::Custom:     return "Custom";
        default:                   return "Unknown";
    }
}
GameType ProjectContext::StringToGameType(const std::string& str) {
    if (str == "RPG")        return GameType::RPG;
    if (str == "FPS")        return GameType::FPS;
    if (str == "TPS")        return GameType::TPS;
    if (str == "Strategy")   return GameType::Strategy;
    if (str == "Platformer") return GameType::Platformer;
    if (str == "Puzzle")     return GameType::Puzzle;
    if (str == "Simulation") return GameType::Simulation;
    if (str == "Racing")     return GameType::Racing;
    if (str == "Adventure")  return GameType::Adventure;
    if (str == "Survival")   return GameType::Survival;
    if (str == "Custom")     return GameType::Custom;
    return GameType::Unknown;
}
const char* ProjectContext::CategoryToString(AssetCategory cat) {
    switch (cat) {
        case AssetCategory::Mesh3D:      return "Mesh3D";
        case AssetCategory::Mesh2D:      return "Mesh2D";
        case AssetCategory::Texture:     return "Texture";
        case AssetCategory::Terrain:     return "Terrain";
        case AssetCategory::Character:   return "Character";
        case AssetCategory::NPC:         return "NPC";
        case AssetCategory::Environment: return "Environment";
        case AssetCategory::Prop:        return "Prop";
        case AssetCategory::VFX:         return "VFX";
        case AssetCategory::Audio:       return "Audio";
        case AssetCategory::Music:       return "Music";
        case AssetCategory::UI:          return "UI";
        case AssetCategory::Animation:   return "Animation";
        case AssetCategory::Script:      return "Script";
        case AssetCategory::Prefab:      return "Prefab";
        case AssetCategory::World:       return "World";
        default:                         return "Unknown";
    }
}
AssetCategory ProjectContext::StringToCategory(const std::string& str) {
    if (str == "Mesh3D")      return AssetCategory::Mesh3D;
    if (str == "Mesh2D")      return AssetCategory::Mesh2D;
    if (str == "Texture")     return AssetCategory::Texture;
    if (str == "Terrain")     return AssetCategory::Terrain;
    if (str == "Character")   return AssetCategory::Character;
    if (str == "NPC")         return AssetCategory::NPC;
    if (str == "Environment") return AssetCategory::Environment;
    if (str == "Prop")        return AssetCategory::Prop;
    if (str == "VFX")         return AssetCategory::VFX;
    if (str == "Audio")       return AssetCategory::Audio;
    if (str == "Music")       return AssetCategory::Music;
    if (str == "UI")          return AssetCategory::UI;
    if (str == "Animation")   return AssetCategory::Animation;
    if (str == "Script")      return AssetCategory::Script;
    if (str == "Prefab")      return AssetCategory::Prefab;
    if (str == "World")       return AssetCategory::World;
    return AssetCategory::Unknown;
}

void ProjectContext::UpdateRequirementsFulfillment() {
    for (auto& req : m_requirements) {
        req.fulfilled = HasAssetCategory(req.category);
    }
}

std::vector<AssetRequirement> ProjectContext::GenerateDefaultRequirements(GameType type) const {
    std::vector<AssetRequirement> reqs;
    AssetRequirement r;
    r.category = AssetCategory::Texture; r.name = "Base Textures"; r.priority = 10;
    reqs.push_back(r);
    if (type == GameType::RPG || type == GameType::Adventure || type == GameType::Survival) {
        r.category = AssetCategory::Character; r.name = "Player Character"; r.priority = 20;
        reqs.push_back(r);
        r.category = AssetCategory::NPC; r.name = "NPCs"; r.priority = 15;
        reqs.push_back(r);
    }
    if (type == GameType::FPS || type == GameType::TPS) {
        r.category = AssetCategory::Character; r.name = "Player Model"; r.priority = 20;
        reqs.push_back(r);
    }
    return reqs;
}

} // namespace AI
