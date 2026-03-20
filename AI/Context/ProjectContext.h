#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

namespace AI {

enum class GameType : uint8_t {
    Unknown, RPG, FPS, TPS, Strategy, Platformer, Puzzle,
    Simulation, Racing, Adventure, Survival, Custom
};

enum class AssetCategory : uint8_t {
    Unknown, Mesh3D, Mesh2D, Texture, Terrain, Character, NPC,
    Environment, Prop, VFX, Audio, Music, UI, Animation, Script, Prefab, World
};

struct AssetRequirement {
    AssetCategory            category    = AssetCategory::Unknown;
    std::string              name;
    std::string              description;
    uint32_t                 priority    = 0;
    bool                     fulfilled   = false;
    std::vector<std::string> matchingAssetIds;
};

struct AssetInventory {
    std::unordered_map<AssetCategory, uint32_t> categoryCounts;
    std::unordered_set<std::string>             assetIds;
    uint64_t                                    totalAssetCount = 0;
    uint64_t                                    totalSizeBytes  = 0;
};

struct AssetSuggestion {
    AssetCategory category    = AssetCategory::Unknown;
    std::string   name;
    std::string   description;
    std::string   source;
    double        confidence  = 0.0;
    uint32_t      priority    = 0;
};

class ProjectContext {
public:
    ProjectContext();

    void      SetGameType(GameType type);
    GameType  GetGameType() const;
    void      DetectGameType();

    void              SetProjectName(const std::string& name);
    const std::string& GetProjectName() const;
    void              SetProjectRoot(const std::string& root);
    const std::string& GetProjectRoot() const;

    void     ScanAssets();
    void     AddAsset(const std::string& id, AssetCategory category);
    void     RemoveAsset(const std::string& id);
    bool     HasAsset(const std::string& id) const;
    bool     HasAssetCategory(AssetCategory category) const;
    uint32_t GetAssetCount(AssetCategory category) const;
    const AssetInventory& GetInventory() const;

    void                                  AddRequirement(const AssetRequirement& req);
    void                                  ClearRequirements();
    void                                  AnalyzeRequirements();
    const std::vector<AssetRequirement>&  GetRequirements() const;
    std::vector<AssetRequirement>         GetUnfulfilledRequirements() const;

    std::vector<AssetSuggestion> GenerateSuggestions() const;
    std::vector<AssetSuggestion> GetPrioritySuggestions(uint32_t maxCount) const;

    void        Clear();
    std::string ToJson() const;
    bool        FromJson(const std::string& json);

    static const char*   GameTypeToString(GameType type);
    static GameType      StringToGameType(const std::string& str);
    static const char*   CategoryToString(AssetCategory category);
    static AssetCategory StringToCategory(const std::string& str);

private:
    void                          UpdateRequirementsFulfillment();
    std::vector<AssetRequirement> GenerateDefaultRequirements(GameType type) const;

    GameType                                       m_gameType = GameType::Unknown;
    std::string                                    m_projectName;
    std::string                                    m_projectRoot;
    AssetInventory                                 m_inventory;
    std::vector<AssetRequirement>                  m_requirements;
    std::unordered_map<std::string, AssetCategory> m_assetCategories;
};

} // namespace AI
