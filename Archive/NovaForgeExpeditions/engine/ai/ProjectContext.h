#pragma once
// ============================================================
// Atlas Project Context — AI Decision Support
// ============================================================
//
// Provides a comprehensive view of the current project state
// for AI-driven asset decision making. The context includes:
//
// - Game type classification (RPG, FPS, Strategy, etc.)
// - Current asset inventory
// - Missing/required asset analysis
// - Procedural generation parameters
//
// This enables the AI to make intelligent suggestions about
// what assets to generate or fetch from external sources.
//
// See: docs/16_ATLAS_AI.md

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

namespace atlas::ai {

/// Game type classification for context-aware asset suggestions
enum class GameType : uint8_t {
    Unknown,     // Game type not determined
    RPG,         // Role-playing game
    FPS,         // First-person shooter
    TPS,         // Third-person shooter
    Strategy,    // Strategy game (RTS/TBS)
    Platformer,  // 2D/3D platformer
    Puzzle,      // Puzzle game
    Simulation,  // Simulation/sandbox
    Racing,      // Racing game
    Adventure,   // Adventure/exploration
    Survival,    // Survival game
    Custom       // Custom/mixed genre
};

/// Asset category for requirement tracking
enum class AssetCategory : uint8_t {
    Unknown,
    Mesh3D,          // 3D models
    Mesh2D,          // 2D sprites
    Texture,         // Textures and materials
    Terrain,         // Terrain data
    Character,       // Character assets
    NPC,             // NPC models/data
    Environment,     // Environment objects
    Prop,            // Props and decorations
    VFX,             // Visual effects
    Audio,           // Sound effects
    Music,           // Background music
    UI,              // UI elements
    Animation,       // Animation data
    Script,          // Gameplay scripts
    Prefab,          // Prefab compositions
    World,           // World/level data
};

/// Represents a requirement for a specific asset type
struct AssetRequirement {
    AssetCategory category = AssetCategory::Unknown;
    std::string name;                // Human-readable name
    std::string description;         // Description of what's needed
    uint32_t priority = 0;           // Priority (higher = more important)
    bool fulfilled = false;          // Whether this requirement is met
    std::vector<std::string> matchingAssetIds; // IDs of assets that fulfill this
};

/// Current state of assets in the project
struct AssetInventory {
    std::unordered_map<AssetCategory, uint32_t> categoryCounts;
    std::unordered_set<std::string> assetIds;
    uint64_t totalAssetCount = 0;
    uint64_t totalSizeBytes = 0;
};

/// Suggestion for a missing or recommended asset
struct AssetSuggestion {
    AssetCategory category = AssetCategory::Unknown;
    std::string name;
    std::string description;
    std::string source;              // "procedural", "marketplace:itch.io", etc.
    double confidence = 0.0;         // AI confidence in this suggestion
    uint32_t priority = 0;           // Suggested priority
};

/// Context snapshot for AI decision making
class ProjectContext {
public:
    ProjectContext();

    // --- Game Type ---
    void SetGameType(GameType type);
    GameType GetGameType() const;
    void DetectGameType();           // Auto-detect from project structure

    // --- Project Info ---
    void SetProjectName(const std::string& name);
    const std::string& GetProjectName() const;
    void SetProjectRoot(const std::string& root);
    const std::string& GetProjectRoot() const;

    // --- Asset Inventory ---
    void ScanAssets();               // Scan project for current assets
    void AddAsset(const std::string& id, AssetCategory category);
    void RemoveAsset(const std::string& id);
    bool HasAsset(const std::string& id) const;
    bool HasAssetCategory(AssetCategory category) const;
    uint32_t GetAssetCount(AssetCategory category) const;
    const AssetInventory& GetInventory() const;

    // --- Requirements ---
    void AddRequirement(const AssetRequirement& req);
    void ClearRequirements();
    void AnalyzeRequirements();      // Auto-detect requirements from game type
    const std::vector<AssetRequirement>& GetRequirements() const;
    std::vector<AssetRequirement> GetUnfulfilledRequirements() const;

    // --- Suggestions ---
    std::vector<AssetSuggestion> GenerateSuggestions() const;
    std::vector<AssetSuggestion> GetPrioritySuggestions(uint32_t maxCount) const;

    // --- Serialization ---
    void Clear();
    std::string ToJson() const;
    bool FromJson(const std::string& json);

    // --- Static Helpers ---
    static const char* GameTypeToString(GameType type);
    static GameType StringToGameType(const std::string& str);
    static const char* CategoryToString(AssetCategory category);
    static AssetCategory StringToCategory(const std::string& str);

private:
    void UpdateRequirementsFulfillment();
    std::vector<AssetRequirement> GenerateDefaultRequirements(GameType type) const;

    GameType m_gameType = GameType::Unknown;
    std::string m_projectName;
    std::string m_projectRoot;
    AssetInventory m_inventory;
    std::vector<AssetRequirement> m_requirements;
    std::unordered_map<std::string, AssetCategory> m_assetCategories;
};

} // namespace atlas::ai
