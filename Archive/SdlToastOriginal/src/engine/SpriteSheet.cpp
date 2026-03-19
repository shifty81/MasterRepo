#include "SpriteSheet.h"
#include "Renderer.h"
#include "Logger.h"
#include <iostream>
#include <algorithm>
#include <vector>

// For directory reading
#ifndef _WIN32
#include <dirent.h>
#endif

// ============================================================================
// SpriteSheet Implementation
// ============================================================================

SpriteSheet::SpriteSheet()
    : m_texture{}
    , m_tileWidth(0)
    , m_tileHeight(0)
    , m_columns(0)
    , m_rows(0)
    , m_sheetWidth(0)
    , m_sheetHeight(0)
{
}

SpriteSheet::~SpriteSheet() {
    if (m_texture.id != 0) {
        UnloadTexture(m_texture);
        m_texture.id = 0;
    }
}

bool SpriteSheet::Load(Renderer* /*renderer*/, const std::string& filepath, int tileWidth, int tileHeight) {
    // Load the image
    Image image = LoadImage(filepath.c_str());
    if (image.data == nullptr) {
        Logger::Instance().Error("Failed to load sprite sheet " + filepath);
        return false;
    }

    m_sheetWidth = image.width;
    m_sheetHeight = image.height;
    m_tileWidth = tileWidth;
    m_tileHeight = tileHeight;
    m_columns = m_sheetWidth / m_tileWidth;
    m_rows = m_sheetHeight / m_tileHeight;

    // Create texture from image
    m_texture = LoadTextureFromImage(image);
    UnloadImage(image);

    if (m_texture.id == 0) {
        Logger::Instance().Error("Failed to create texture from " + filepath);
        return false;
    }

    Logger::Instance().Info("Loaded sprite sheet: " + filepath);
    Logger::Instance().Info("  Size: " + std::to_string(m_sheetWidth) + "x" + std::to_string(m_sheetHeight));
    Logger::Instance().Info("  Tiles: " + std::to_string(m_columns) + "x" + std::to_string(m_rows) + " (" + std::to_string(GetTileCount()) + " total)");
    Logger::Instance().Info("  Tile size: " + std::to_string(m_tileWidth) + "x" + std::to_string(m_tileHeight));

    return true;
}

void SpriteSheet::RenderTile(Renderer* renderer, int tileId, int x, int y, bool flipH, bool flipV) {
    RenderTile(renderer, tileId, x, y, m_tileWidth, m_tileHeight, flipH, flipV);
}

void SpriteSheet::RenderTile(Renderer* renderer, int tileId, int x, int y, int width, int height, bool flipH, bool flipV) {
    if (m_texture.id == 0) return;

    Rectangle srcRect;
    CalculateSourceRect(tileId, srcRect);

    // Apply flipping by negating source dimensions
    if (flipH) srcRect.width = -srcRect.width;
    if (flipV) srcRect.height = -srcRect.height;

    int camX, camY;
    renderer->GetCamera(camX, camY);

    Rectangle dstRect = { 
        static_cast<float>(x - camX), 
        static_cast<float>(y - camY), 
        static_cast<float>(width), 
        static_cast<float>(height) 
    };

    DrawTexturePro(m_texture, srcRect, dstRect, Vector2{0, 0}, 0.0f, WHITE);
}

void SpriteSheet::CalculateSourceRect(int tileId, Rectangle& srcRect) {
    if (m_columns <= 0 || m_tileWidth <= 0 || m_tileHeight <= 0) {
        srcRect = {0, 0, 1.0f, 1.0f};
        return;
    }
    int col = tileId % m_columns;
    int row = tileId / m_columns;
    
    srcRect.x = static_cast<float>(col * m_tileWidth);
    srcRect.y = static_cast<float>(row * m_tileHeight);
    srcRect.width = static_cast<float>(m_tileWidth);
    srcRect.height = static_cast<float>(m_tileHeight);
}

// ============================================================================
// SpriteSheetManager Implementation
// ============================================================================

SpriteSheetManager& SpriteSheetManager::Instance() {
    static SpriteSheetManager instance;
    return instance;
}

SpriteSheetManager::~SpriteSheetManager() {
    Clear();
}

SpriteSheet* SpriteSheetManager::LoadSpriteSheet(Renderer* renderer, const std::string& name, 
                                                  const std::string& filepath, int tileWidth, int tileHeight) {
    // Check if already loaded
    auto it = m_sheets.find(name);
    if (it != m_sheets.end()) {
        return it->second;
    }

    // Load new sprite sheet
    SpriteSheet* sheet = new SpriteSheet();
    if (sheet->Load(renderer, filepath, tileWidth, tileHeight)) {
        m_sheets[name] = sheet;
        return sheet;
    } else {
        delete sheet;
        return nullptr;
    }
}

SpriteSheet* SpriteSheetManager::GetSpriteSheet(const std::string& name) {
    auto it = m_sheets.find(name);
    return (it != m_sheets.end()) ? it->second : nullptr;
}

void SpriteSheetManager::LoadDefaultAssets(Renderer* renderer) {
    Logger::Instance().Info("");
    Logger::Instance().Info("=== Loading Sprite Sheets ===");
    
    // Check root directory for PNG files first (user convenience)
    Logger::Instance().Info("Checking root directory for PNG files...");
    bool foundRootPngs = CheckAndLoadRootPngs(renderer);
    if (foundRootPngs) {
        Logger::Instance().Info("Loaded tilesets from root directory!");
    }
    
    // Load world tileset (32x32 tiles)
    if (!GetSpriteSheet("world_tiles")) {
        LoadSpriteSheet(renderer, "world_tiles", "assets/tilesets/world_tileset.png", 32, 32);
    }
    
    // Load character sprite sheet (16x16 or 32x32)
    if (!GetSpriteSheet("characters")) {
        LoadSpriteSheet(renderer, "characters", "assets/sprites/character_tileset.png", 16, 16);
    }
    
    // Load additional tilesets as needed
    LoadSpriteSheet(renderer, "dungeon_tiles", "assets/tilesets/dungeon_tileset.png", 16, 16);
    LoadSpriteSheet(renderer, "farm_tiles", "assets/tilesets/farm_tileset.png", 16, 16);
    
    Logger::Instance().Info("=== Sprite Sheets Loaded ===");
}

bool SpriteSheetManager::CheckAndLoadRootPngs(Renderer* renderer) {
    // Common tileset filenames to check in root directory
    std::vector<std::string> tilesetNames = {
        "world_tileset.png", "tiles.png", "tileset.png", "world.png",
        "terrain.png", "map.png"
    };
    
    std::vector<std::string> characterNames = {
        "character_tileset.png", "characters.png", "char.png", "sprites.png",
        "player.png", "hero.png"
    };
    
    bool loadedAny = false;
    
    // Try to load world tileset from root
    for (const auto& name : tilesetNames) {
        if (LoadSpriteSheet(renderer, "world_tiles", name, 32, 32)) {
            Logger::Instance().Info("  ✓ Loaded world tiles from: " + name);
            loadedAny = true;
            break;
        }
    }
    
    // Try to load character sprites from root
    for (const auto& name : characterNames) {
        if (LoadSpriteSheet(renderer, "characters", name, 16, 16)) {
            Logger::Instance().Info("  ✓ Loaded character sprites from: " + name);
            loadedAny = true;
            break;
        }
    }
    
    // If exactly 2 PNGs in root, try them in order
    // (This handles the case where user just drops 2 PNGs with any names)
    if (!loadedAny) {
        std::vector<std::string> rootPngs;
        // Check for any .png files in current directory
        #ifdef _WIN32
        // Windows version would use different API
        #else
        // Unix-like systems
        DIR* dir = opendir(".");
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                std::string filename = entry->d_name;
                if (filename.length() > 4 && 
                    filename.substr(filename.length() - 4) == ".png") {
                    rootPngs.push_back(filename);
                }
            }
            closedir(dir);
        }
        #endif
        
        // Sort for consistency
        std::sort(rootPngs.begin(), rootPngs.end());
        
        if (rootPngs.size() >= 1 && !GetSpriteSheet("world_tiles")) {
            if (LoadSpriteSheet(renderer, "world_tiles", rootPngs[0], 32, 32)) {
                Logger::Instance().Info("  ✓ Loaded world tiles from: " + rootPngs[0] + " (1st PNG in root)");
                loadedAny = true;
            }
        }
        
        if (rootPngs.size() >= 2 && !GetSpriteSheet("characters")) {
            if (LoadSpriteSheet(renderer, "characters", rootPngs[1], 16, 16)) {
                Logger::Instance().Info("  ✓ Loaded character sprites from: " + rootPngs[1] + " (2nd PNG in root)");
                loadedAny = true;
            }
        }
    }
    
    return loadedAny;
}

void SpriteSheetManager::Clear() {
    for (auto& pair : m_sheets) {
        delete pair.second;
    }
    m_sheets.clear();
}