#include "Tile.h"

// Initialize static members
TileDef TileRegistry::s_definitions[32];
bool TileRegistry::s_initialized = false;

void TileRegistry::Initialize() {
    if (s_initialized) return;
    
    // Define tile rules - the SMART data
    s_definitions[static_cast<int>(TileType::VOID)]       = TileDef(TileType::VOID,       true,  false, false, "void");
    s_definitions[static_cast<int>(TileType::FLOOR)]      = TileDef(TileType::FLOOR,      false, false, false, "floor");
    s_definitions[static_cast<int>(TileType::WALL)]       = TileDef(TileType::WALL,       true,  false, false, "wall");
    s_definitions[static_cast<int>(TileType::DOOR)]       = TileDef(TileType::DOOR,       false, false, false, "door");
    s_definitions[static_cast<int>(TileType::WATER)]      = TileDef(TileType::WATER,      true,  false, false, "water");
    s_definitions[static_cast<int>(TileType::SOIL)]       = TileDef(TileType::SOIL,       false, false, true,  "soil");
    s_definitions[static_cast<int>(TileType::CROP)]       = TileDef(TileType::CROP,       false, false, false, "crop");
    s_definitions[static_cast<int>(TileType::GRASS)]      = TileDef(TileType::GRASS,      false, true,  false, "grass");
    s_definitions[static_cast<int>(TileType::DIRT)]       = TileDef(TileType::DIRT,       false, false, false, "dirt");
    s_definitions[static_cast<int>(TileType::STONE)]      = TileDef(TileType::STONE,      false, false, false, "stone");
    s_definitions[static_cast<int>(TileType::SAND)]       = TileDef(TileType::SAND,       false, false, false, "sand");
    s_definitions[static_cast<int>(TileType::DECORATION)] = TileDef(TileType::DECORATION, false, true,  false, "decoration");
    s_definitions[static_cast<int>(TileType::TREE)]       = TileDef(TileType::TREE,       true,  true,  false, "tree");
    
    s_initialized = true;
}

const TileDef& TileRegistry::GetDefinition(TileType type) {
    if (!s_initialized) Initialize();
    return s_definitions[static_cast<int>(type)];
}

// Tile implementation
Tile::Tile()
    : m_type(TileType::GRASS)
    , m_visualId(0)
    , m_soilState(SoilState::GRASS)
    , m_cropType(-1)
    , m_growthStage(0)
    , m_growthTimer(0.0f)
{
}

Tile::Tile(TileType type, int visualId)
    : m_type(type)
    , m_visualId(visualId)
    , m_soilState(SoilState::GRASS)
    , m_cropType(-1)
    , m_growthStage(0)
    , m_growthTimer(0.0f)
{
}

bool Tile::IsSolid() const {
    return TileRegistry::GetDefinition(m_type).solid;
}

bool Tile::IsBreakable() const {
    return TileRegistry::GetDefinition(m_type).breakable;
}

bool Tile::IsFarmable() const {
    return TileRegistry::GetDefinition(m_type).farmable;
}

void Tile::Update(float deltaTime) {
    // Crop growth over time
    if (m_soilState == SoilState::CROP && m_cropType >= 0 &&
        m_growthStage < MAX_GROWTH_STAGE) {
        m_growthTimer += deltaTime;
        if (m_growthTimer >= GROWTH_INTERVAL) {
            m_growthTimer -= GROWTH_INTERVAL;
            m_growthStage++;
            if (m_growthStage >= MAX_GROWTH_STAGE) {
                m_soilState = SoilState::HARVEST;
            }
        }
    }
}
