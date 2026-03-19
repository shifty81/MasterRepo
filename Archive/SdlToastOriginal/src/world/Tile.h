#ifndef TILE_H
#define TILE_H

#include <string>

// Semantic tile types - what the tile MEANS, not how it looks
enum class TileType {
    VOID,           // Empty/ungenerated space
    FLOOR,          // Walkable floor
    WALL,           // Solid wall
    DOOR,           // Door (can be open/closed)
    WATER,          // Water (not walkable)
    SOIL,           // Farmable soil
    CROP,           // Growing crop
    GRASS,          // Grass terrain
    DIRT,           // Dirt path
    STONE,          // Stone floor
    SAND,           // Sand terrain
    DECORATION,     // Non-interactive decoration
    TREE            // Tree (solid, choppable)
};

// Tile definition - the RULES for a tile type
struct TileDef {
    TileType type;
    bool solid;         // Blocks movement
    bool breakable;     // Can be destroyed (pots, grass, etc.)
    bool farmable;      // Can plant crops here
    std::string name;
    
    TileDef() 
        : type(TileType::VOID), solid(false), breakable(false), farmable(false), name("void") {}
    
    TileDef(TileType t, bool s, bool b, bool f, const std::string& n)
        : type(t), solid(s), breakable(b), farmable(f), name(n) {}
};

// Farm-specific tile state
enum class SoilState {
    GRASS,      // Untilled
    HOE,        // Tilled, dry
    WATERED,    // Tilled, watered
    CROP,       // Has crop growing
    HARVEST     // Ready to harvest
};

// Individual tile instance in the world
class Tile {
public:
    Tile();
    Tile(TileType type, int visualId = 0);
    
    // Tile semantics (what it IS)
    TileType GetType() const { return m_type; }
    void SetType(TileType type) { m_type = type; }
    
    // Visual representation (what it LOOKS like)
    int GetVisualId() const { return m_visualId; }
    void SetVisualId(int id) { m_visualId = id; }
    
    // Properties from definition
    bool IsSolid() const;
    bool IsBreakable() const;
    bool IsFarmable() const;
    
    // Farming system
    SoilState GetSoilState() const { return m_soilState; }
    void SetSoilState(SoilState state) { m_soilState = state; }
    
    int GetCropType() const { return m_cropType; }
    void SetCropType(int type) { m_cropType = type; }
    
    int GetGrowthStage() const { return m_growthStage; }
    void SetGrowthStage(int stage) { m_growthStage = stage; }
    
    // Update (for animated tiles, crop growth, etc.)
    void Update(float deltaTime);

    // Crop growth constants
    static constexpr int MAX_GROWTH_STAGE = 4;
    static constexpr float GROWTH_INTERVAL = 5.0f; // Seconds between growth stages

private:
    TileType m_type;        // Semantic type
    int m_visualId;         // Which sprite to render
    
    // Farming state (only used if farmable)
    SoilState m_soilState;
    int m_cropType;         // -1 = no crop
    int m_growthStage;      // 0-N growth stages
    float m_growthTimer;    // Time accumulator for crop growth
};

// Global tile definition registry
class TileRegistry {
public:
    static void Initialize();
    static const TileDef& GetDefinition(TileType type);
    
private:
    static TileDef s_definitions[32];
    static bool s_initialized;
};

#endif // TILE_H
