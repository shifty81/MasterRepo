#include "TilesetConfig.h"
#include "../world/Tile.h"
#include "../systems/Calendar.h"
#include <fstream>
#include <sstream>
#include <iostream>

TilesetConfig::TilesetConfig()
    : m_wallAutoTileBase(7)
    , m_cropGrowthBase(30)
    , m_decorationBase(40)
    , m_treeBase(50)
    , m_waterFrameCount(4)
    , m_tileSize(32)
    , m_hasSeasonal(false)
{
    m_waterFrames[0] = 3;
    m_waterFrames[1] = 21;
    m_waterFrames[2] = 22;
    m_waterFrames[3] = 23;
    for (int i = 4; i < MAX_WATER_FRAMES; ++i) m_waterFrames[i] = 0;
}

void TilesetConfig::LoadDefaults() {
    m_baseIds.clear();
    m_seasonalIds.clear();

    // Default mapping — matches the existing world_tileset.png layout
    m_baseIds[static_cast<int>(TileType::GRASS)]      = 0;
    m_baseIds[static_cast<int>(TileType::DIRT)]        = 1;
    m_baseIds[static_cast<int>(TileType::SOIL)]        = 2;
    m_baseIds[static_cast<int>(TileType::WATER)]       = 3;
    m_baseIds[static_cast<int>(TileType::STONE)]       = 4;
    m_baseIds[static_cast<int>(TileType::SAND)]        = 5;
    m_baseIds[static_cast<int>(TileType::FLOOR)]       = 6;
    m_baseIds[static_cast<int>(TileType::WALL)]        = 7;
    m_baseIds[static_cast<int>(TileType::DOOR)]        = 20;
    m_baseIds[static_cast<int>(TileType::CROP)]        = 30;
    m_baseIds[static_cast<int>(TileType::DECORATION)]  = 40;
    m_baseIds[static_cast<int>(TileType::TREE)]        = 50;
    m_baseIds[static_cast<int>(TileType::VOID)]        = 0;

    m_wallAutoTileBase = 7;
    m_cropGrowthBase   = 30;
    m_decorationBase   = 40;
    m_treeBase         = 50;

    m_waterFrames[0] = 3;
    m_waterFrames[1] = 21;
    m_waterFrames[2] = 22;
    m_waterFrames[3] = 23;
    m_waterFrameCount = 4;

    m_tileSize = 32;
    m_sheetFile = "assets/tilesets/world_tileset.png";
    m_hasSeasonal = false;
}

bool TilesetConfig::LoadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cout << "TilesetConfig: Cannot open " << filepath << std::endl;
        return false;
    }

    // Start with defaults, then override
    LoadDefaults();

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string key;
        iss >> key;

        if (key == "TILE_SIZE") {
            iss >> m_tileSize;
        } else if (key == "SHEET_FILE") {
            // Read rest of line to support paths with spaces
            std::string rest;
            std::getline(iss, rest);
            if (!rest.empty() && rest[0] == ' ') rest = rest.substr(1);
            if (!rest.empty()) m_sheetFile = rest;
        } else if (key == "WALL_AUTOTILE_BASE") {
            iss >> m_wallAutoTileBase;
        } else if (key == "CROP_GROWTH_BASE") {
            iss >> m_cropGrowthBase;
        } else if (key == "DECORATION_BASE") {
            iss >> m_decorationBase;
        } else if (key == "TREE_BASE") {
            iss >> m_treeBase;
        } else if (key == "WATER_ANIM_FRAMES") {
            m_waterFrameCount = 0;
            int frame;
            while (iss >> frame && m_waterFrameCount < MAX_WATER_FRAMES) {
                m_waterFrames[m_waterFrameCount++] = frame;
            }
        } else {
            // Tile type mapping: NAME ID or NAME.SEASON ID
            size_t dotPos = key.find('.');
            if (dotPos != std::string::npos) {
                // Seasonal override: GRASS.SPRING 16
                std::string typeName = key.substr(0, dotPos);
                std::string seasonName = key.substr(dotPos + 1);

                int typeInt = ParseTileType(typeName);
                int seasonInt = ParseSeason(seasonName);
                int spriteId = 0;
                iss >> spriteId;

                if (typeInt >= 0 && seasonInt >= 0) {
                    SeasonKey sk{typeInt, seasonInt};
                    m_seasonalIds[sk] = spriteId;
                    m_hasSeasonal = true;
                }
            } else {
                // Base mapping: GRASS 0
                int typeInt = ParseTileType(key);
                int spriteId = 0;
                iss >> spriteId;

                if (typeInt >= 0) {
                    m_baseIds[typeInt] = spriteId;
                }
            }
        }
    }

    file.close();
    std::cout << "TilesetConfig: Loaded from " << filepath
              << " (tile_size=" << m_tileSize
              << ", seasonal=" << (m_hasSeasonal ? "yes" : "no") << ")" << std::endl;
    return true;
}

int TilesetConfig::GetSpriteId(TileType type, Season season) const {
    int typeInt = static_cast<int>(type);
    int seasonInt = static_cast<int>(season);

    // Check seasonal override first
    if (m_hasSeasonal) {
        SeasonKey sk{typeInt, seasonInt};
        auto it = m_seasonalIds.find(sk);
        if (it != m_seasonalIds.end()) {
            return it->second;
        }
    }

    // Fall back to base ID
    return GetSpriteId(type);
}

int TilesetConfig::GetSpriteId(TileType type) const {
    int typeInt = static_cast<int>(type);
    auto it = m_baseIds.find(typeInt);
    if (it != m_baseIds.end()) {
        return it->second;
    }
    return 0; // Default
}

int TilesetConfig::GetWaterFrame(int frameIndex) const {
    if (frameIndex < 0 || frameIndex >= m_waterFrameCount) return m_waterFrames[0];
    return m_waterFrames[frameIndex];
}

int TilesetConfig::ParseTileType(const std::string& name) {
    if (name == "VOID")       return static_cast<int>(TileType::VOID);
    if (name == "FLOOR")      return static_cast<int>(TileType::FLOOR);
    if (name == "WALL")       return static_cast<int>(TileType::WALL);
    if (name == "DOOR")       return static_cast<int>(TileType::DOOR);
    if (name == "WATER")      return static_cast<int>(TileType::WATER);
    if (name == "SOIL")       return static_cast<int>(TileType::SOIL);
    if (name == "CROP")       return static_cast<int>(TileType::CROP);
    if (name == "GRASS")      return static_cast<int>(TileType::GRASS);
    if (name == "DIRT")       return static_cast<int>(TileType::DIRT);
    if (name == "STONE")      return static_cast<int>(TileType::STONE);
    if (name == "SAND")       return static_cast<int>(TileType::SAND);
    if (name == "DECORATION") return static_cast<int>(TileType::DECORATION);
    if (name == "TREE")       return static_cast<int>(TileType::TREE);
    return -1;
}

int TilesetConfig::ParseSeason(const std::string& name) {
    if (name == "SPRING") return static_cast<int>(Season::SPRING);
    if (name == "SUMMER") return static_cast<int>(Season::SUMMER);
    if (name == "FALL")   return static_cast<int>(Season::FALL);
    if (name == "WINTER") return static_cast<int>(Season::WINTER);
    return -1;
}
