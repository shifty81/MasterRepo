#include "WorldGenerator.h"
#include "Map.h"
#include <cmath>
#include <algorithm>

WorldGenerator::WorldGenerator(unsigned int seed)
    : m_seed(seed == 0 ? std::random_device{}() : seed)
    , m_rng(m_seed)
{
}

void WorldGenerator::SetSeed(unsigned int seed) {
    m_seed = seed;
    m_rng.seed(m_seed);
}

// ============================================================================
// FARM GENERATION (Stardew-Style)
// ============================================================================

void WorldGenerator::GenerateFarm(Map* map, int width, int height) {
    // STEP 1: Generate farm zones (logical layout)
    auto zones = GenerateFarmZones(width, height);
    
    // STEP 2: Fill base with grass
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            map->SetTile(x, y, Tile(TileType::GRASS, 0));
        }
    }
    
    // STEP 3: Apply zones
    for (const auto& zone : zones) {
        ApplyFarmZone(map, zone);
    }
    
    // STEP 4: Auto-tile (make it look good)
    ApplyAutoTiling(map);
    
    // STEP 5: Add decorations
    AddDecorations(map, 0.05f);
    
    // STEP 6: Add scattered trees around the farm edges
    AddTrees(map, 0.15f);
}

std::vector<WorldGenerator::FarmZone> WorldGenerator::GenerateFarmZones(int width, int height) {
    std::vector<FarmZone> zones;
    
    // Central farmland for crops
    zones.push_back({
        width / 4, height / 4,
        width / 2, height / 2,
        FarmZone::SOIL
    });
    
    // Water source
    zones.push_back({
        width - 8, height / 2 - 3,
        6, 6,
        FarmZone::WATER
    });
    
    // Paths around farm
    zones.push_back({
        0, height / 2,
        width, 2,
        FarmZone::PATH
    });
    
    return zones;
}

void WorldGenerator::ApplyFarmZone(Map* map, const FarmZone& zone) {
    for (int y = zone.y; y < zone.y + zone.height && y < map->GetHeight(); ++y) {
        for (int x = zone.x; x < zone.x + zone.width && x < map->GetWidth(); ++x) {
            switch (zone.type) {
                case FarmZone::SOIL:
                    map->SetTile(x, y, Tile(TileType::SOIL, 0));
                    break;
                case FarmZone::WATER:
                    map->SetTile(x, y, Tile(TileType::WATER, 0));
                    break;
                case FarmZone::PATH:
                    map->SetTile(x, y, Tile(TileType::DIRT, 0));
                    break;
                case FarmZone::BUILDING:
                    map->SetTile(x, y, Tile(TileType::FLOOR, 0));
                    break;
            }
        }
    }
}

// ============================================================================
// DUNGEON GENERATION (Zelda-Style)
// ============================================================================

void WorldGenerator::GenerateDungeon(Map* map, int width, int height) {
    // STEP 1: Fill with void/walls
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            map->SetTile(x, y, Tile(TileType::WALL, 0));
        }
    }
    
    // STEP 2: Generate rooms (logical structure)
    auto rooms = GenerateRooms(width, height);
    
    // STEP 3: Carve rooms
    for (const auto& room : rooms) {
        CarveRoom(map, room);
    }
    
    // STEP 4: Connect rooms with corridors
    auto corridors = ConnectRooms(rooms);
    for (const auto& corridor : corridors) {
        CarveCorridor(map, corridor);
    }
    
    // STEP 5: Place doors at room entrances (where corridors meet room edges)
    for (const auto& room : rooms) {
        for (int x = room.x; x < room.x + room.width; ++x) {
            // Top edge
            if (room.y > 0) {
                Tile* above = map->GetTileAt(x, room.y - 1);
                Tile* edge = map->GetTileAt(x, room.y);
                if (above && edge && above->GetType() == TileType::FLOOR && edge->GetType() == TileType::FLOOR) {
                    map->SetTile(x, room.y, Tile(TileType::DOOR, 0));
                }
            }
            // Bottom edge
            if (room.y + room.height < map->GetHeight()) {
                Tile* below = map->GetTileAt(x, room.y + room.height);
                Tile* edge = map->GetTileAt(x, room.y + room.height - 1);
                if (below && edge && below->GetType() == TileType::FLOOR && edge->GetType() == TileType::FLOOR) {
                    map->SetTile(x, room.y + room.height - 1, Tile(TileType::DOOR, 0));
                }
            }
        }
        for (int y = room.y; y < room.y + room.height; ++y) {
            // Left edge
            if (room.x > 0) {
                Tile* left = map->GetTileAt(room.x - 1, y);
                Tile* edge = map->GetTileAt(room.x, y);
                if (left && edge && left->GetType() == TileType::FLOOR && edge->GetType() == TileType::FLOOR) {
                    map->SetTile(room.x, y, Tile(TileType::DOOR, 0));
                }
            }
            // Right edge
            if (room.x + room.width < map->GetWidth()) {
                Tile* right = map->GetTileAt(room.x + room.width, y);
                Tile* edge = map->GetTileAt(room.x + room.width - 1, y);
                if (right && edge && right->GetType() == TileType::FLOOR && edge->GetType() == TileType::FLOOR) {
                    map->SetTile(room.x + room.width - 1, y, Tile(TileType::DOOR, 0));
                }
            }
        }
    }
    
    // STEP 6: Auto-tile walls (make it beautiful)
    ApplyAutoTiling(map);
    
    // STEP 7: Add decorations and details
    AddDecorations(map, 0.03f);
}

std::vector<WorldGenerator::Room> WorldGenerator::GenerateRooms(int mapWidth, int mapHeight) {
    std::vector<Room> rooms;
    int numRooms = Random(5, 10);
    int attempts = 0;
    const int maxAttempts = 100;
    
    while (rooms.size() < static_cast<size_t>(numRooms) && attempts < maxAttempts) {
        Room room;
        room.width = Random(5, 12);
        room.height = Random(5, 12);
        room.x = Random(2, mapWidth - room.width - 2);
        room.y = Random(2, mapHeight - room.height - 2);
        room.isBoss = false;
        room.isStart = rooms.empty(); // First room is start
        room.hasTreasure = Random(0, 100) < 30; // 30% chance
        
        // Check for overlap
        bool overlaps = false;
        for (const auto& existingRoom : rooms) {
            if (RoomsOverlap(room, existingRoom)) {
                overlaps = true;
                break;
            }
        }
        
        if (!overlaps) {
            rooms.push_back(room);
        }
        
        attempts++;
    }
    
    // Make last room the boss room
    if (!rooms.empty()) {
        rooms.back().isBoss = true;
    }
    
    return rooms;
}

std::vector<WorldGenerator::Corridor> WorldGenerator::ConnectRooms(const std::vector<Room>& rooms) {
    std::vector<Corridor> corridors;
    
    for (size_t i = 1; i < rooms.size(); ++i) {
        const Room& prev = rooms[i - 1];
        const Room& curr = rooms[i];
        
        int x1 = prev.x + prev.width / 2;
        int y1 = prev.y + prev.height / 2;
        int x2 = curr.x + curr.width / 2;
        int y2 = curr.y + curr.height / 2;
        
        corridors.push_back({x1, y1, x2, y2});
    }
    
    return corridors;
}

void WorldGenerator::CarveRoom(Map* map, const Room& room) {
    for (int y = room.y; y < room.y + room.height; ++y) {
        for (int x = room.x; x < room.x + room.width; ++x) {
            if (x >= 0 && x < map->GetWidth() && y >= 0 && y < map->GetHeight()) {
                map->SetTile(x, y, Tile(TileType::FLOOR, 0));
            }
        }
    }
}

void WorldGenerator::CarveCorridor(Map* map, const Corridor& corridor) {
    int x = corridor.x1;
    int y = corridor.y1;
    
    // Horizontal then vertical
    while (x != corridor.x2) {
        if (x >= 0 && x < map->GetWidth() && y >= 0 && y < map->GetHeight()) {
            map->SetTile(x, y, Tile(TileType::FLOOR, 0));
        }
        x += (corridor.x2 > x) ? 1 : -1;
    }
    
    while (y != corridor.y2) {
        if (x >= 0 && x < map->GetWidth() && y >= 0 && y < map->GetHeight()) {
            map->SetTile(x, y, Tile(TileType::FLOOR, 0));
        }
        y += (corridor.y2 > y) ? 1 : -1;
    }
}

// ============================================================================
// OVERWORLD GENERATION (Noise-based)
// ============================================================================

void WorldGenerator::GenerateOverworld(Map* map, int width, int height, Biome /*biome*/) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float heightValue = Noise2D(x, y, 4);
            float moistureValue = Noise2D(x + 1000, y + 1000, 3);
            
            TileType type = DetermineTileFromNoise(heightValue, moistureValue);
            map->SetTile(x, y, Tile(type, 0));
        }
    }
    
    ApplyAutoTiling(map);
    AddDecorations(map, 0.08f);
    AddTrees(map, 0.12f);
}

float WorldGenerator::Noise2D(int x, int y, int octaves) {
    // Simple perlin-like noise (simplified for demo)
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 0.05f;
    
    for (int i = 0; i < octaves; ++i) {
        float nx = x * frequency;
        float ny = y * frequency;
        
        // Simple pseudo-random based on position
        float n = std::sin(nx * 12.9898f + ny * 78.233f) * 43758.5453f;
        n = n - std::floor(n);
        
        value += n * amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    
    return value / 2.0f; // Normalize roughly to 0-1
}

TileType WorldGenerator::DetermineTileFromNoise(float height, float moisture) {
    if (height < 0.3f) {
        return TileType::WATER;
    } else if (height < 0.5f) {
        return moisture > 0.6f ? TileType::GRASS : TileType::SAND;
    } else if (height < 0.7f) {
        return TileType::GRASS;
    } else {
        return TileType::STONE;
    }
}

// ============================================================================
// AUTO-TILING (The Magic That Makes It Look Hand-Made)
// ============================================================================

void WorldGenerator::ApplyAutoTiling(Map* map) {
    for (int y = 0; y < map->GetHeight(); ++y) {
        for (int x = 0; x < map->GetWidth(); ++x) {
            Tile* tile = map->GetTileAt(x, y);
            if (!tile) continue;
            
            // Auto-tile walls based on neighbors
            if (tile->GetType() == TileType::WALL) {
                bool n = (y > 0 && map->GetTileAt(x, y-1)->GetType() == TileType::WALL);
                bool s = (y < map->GetHeight()-1 && map->GetTileAt(x, y+1)->GetType() == TileType::WALL);
                bool e = (x < map->GetWidth()-1 && map->GetTileAt(x+1, y)->GetType() == TileType::WALL);
                bool w = (x > 0 && map->GetTileAt(x-1, y)->GetType() == TileType::WALL);
                
                bool nw = (x > 0 && y > 0 && map->GetTileAt(x-1, y-1)->GetType() == TileType::WALL);
                bool ne = (x < map->GetWidth()-1 && y > 0 && map->GetTileAt(x+1, y-1)->GetType() == TileType::WALL);
                bool sw = (x > 0 && y < map->GetHeight()-1 && map->GetTileAt(x-1, y+1)->GetType() == TileType::WALL);
                bool se = (x < map->GetWidth()-1 && y < map->GetHeight()-1 && map->GetTileAt(x+1, y+1)->GetType() == TileType::WALL);
                
                int visualId = GetWallAutoTile(n, s, e, w, nw, ne, sw, se);
                tile->SetVisualId(visualId);
            }
        }
    }
}

int WorldGenerator::GetWallAutoTile(bool n, bool s, bool e, bool w,
                                     bool /*nw*/, bool /*ne*/, bool /*sw*/, bool /*se*/) {
    // Simplified auto-tiling algorithm
    // Returns visual ID based on neighbor configuration
    
    if (n && s && e && w) return 0;  // Center tile
    if (!n && s && e && w) return 1; // Top edge
    if (n && !s && e && w) return 2; // Bottom edge
    if (n && s && !e && w) return 3; // Right edge
    if (n && s && e && !w) return 4; // Left edge
    
    if (!n && !s && e && w) return 5;  // Horizontal
    if (n && s && !e && !w) return 6;  // Vertical
    
    if (!n && !e) return 7;  // Top-right corner
    if (!n && !w) return 8;  // Top-left corner
    if (!s && !e) return 9;  // Bottom-right corner
    if (!s && !w) return 10; // Bottom-left corner
    
    return 0; // Default
}

// ============================================================================
// DECORATION PASS
// ============================================================================

void WorldGenerator::AddDecorations(Map* map, float density) {
    for (int y = 1; y < map->GetHeight() - 1; ++y) {
        for (int x = 1; x < map->GetWidth() - 1; ++x) {
            Tile* tile = map->GetTileAt(x, y);
            if (!tile) continue;
            
            // Only decorate grass tiles
            if (tile->GetType() == TileType::GRASS && RandomFloat() < density) {
                // Don't block paths - check neighbors
                bool clearPath = true;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        Tile* neighbor = map->GetTileAt(x + dx, y + dy);
                        if (neighbor && neighbor->GetType() == TileType::DIRT) {
                            clearPath = false;
                            break;
                        }
                    }
                }
                
                if (clearPath) {
                    tile->SetType(TileType::DECORATION);
                    tile->SetVisualId(Random(0, 3)); // Random decoration
                }
            }
        }
    }
}

void WorldGenerator::AddTrees(Map* map, float density) {
    for (int y = 1; y < map->GetHeight() - 1; ++y) {
        for (int x = 1; x < map->GetWidth() - 1; ++x) {
            Tile* tile = map->GetTileAt(x, y);
            if (!tile) continue;

            // Only place trees on grass tiles
            if (tile->GetType() == TileType::GRASS && RandomFloat() < density) {
                // Don't place next to paths, soil, or other trees
                bool clear = true;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        Tile* neighbor = map->GetTileAt(x + dx, y + dy);
                        if (neighbor) {
                            TileType nt = neighbor->GetType();
                            if (nt == TileType::DIRT || nt == TileType::SOIL ||
                                nt == TileType::TREE || nt == TileType::DOOR) {
                                clear = false;
                                break;
                            }
                        }
                    }
                    if (!clear) break;
                }

                if (clear) {
                    tile->SetType(TileType::TREE);
                    tile->SetVisualId(0);
                }
            }
        }
    }
}

// ============================================================================
// UTILITIES
// ============================================================================

bool WorldGenerator::RoomsOverlap(const Room& a, const Room& b) {
    return !(a.x + a.width < b.x ||
             b.x + b.width < a.x ||
             a.y + a.height < b.y ||
             b.y + b.height < a.y);
}

int WorldGenerator::Random(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(m_rng);
}

float WorldGenerator::RandomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(m_rng);
}