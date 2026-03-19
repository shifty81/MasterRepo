#include "WorldStreamer.h"
#include <fstream>
#include <cmath>
#include <algorithm>
#include <filesystem>

namespace atlas::world {

WorldStreamer::WorldStreamer(const WorldLayout& layout, const std::string& cacheDir)
    : m_layout(layout)
    , m_cacheDir(cacheDir)
{
}

uint64_t WorldStreamer::MakeKey(const ChunkCoord& chunk) const {
    return m_layout.MakeChunkID(chunk).value;
}

std::string WorldStreamer::CacheFilePath(const ChunkCoord& chunk) const {
    uint64_t key = MakeKey(chunk);
    return m_cacheDir + "/chunk_" + std::to_string(key) + ".bin";
}

void WorldStreamer::Update(const WorldPos& viewerPos, int lod, float loadRadius, float unloadRadius) {
    // Find chunks within load radius and request them
    ChunkCoord viewerChunk = m_layout.WorldToChunk(viewerPos, lod);

    float chunkSize = m_layout.ChunkWorldSize(lod);
    if (chunkSize <= 0.0f) return;

    int range = static_cast<int>(std::ceil(loadRadius / chunkSize));

    // Request load for chunks within loadRadius
    for (int dz = -range; dz <= range; ++dz) {
        for (int dx = -range; dx <= range; ++dx) {
            ChunkCoord candidate = {viewerChunk.x + dx, viewerChunk.y, viewerChunk.z + dz, lod};
            WorldPos chunkWorld = m_layout.ChunkToWorld(candidate);

            double distX = chunkWorld.x - viewerPos.x;
            double distZ = chunkWorld.z - viewerPos.z;
            double dist = std::sqrt(distX * distX + distZ * distZ);

            if (dist <= loadRadius) {
                RequestLoad(candidate);
            }
        }
    }

    // Unload chunks beyond unloadRadius
    std::vector<uint64_t> toUnload;
    for (auto& [key, entry] : m_chunks) {
        if (entry.state == ChunkState::Loaded) {
            WorldPos chunkWorld = m_layout.ChunkToWorld(entry.coord);
            double distX = chunkWorld.x - viewerPos.x;
            double distZ = chunkWorld.z - viewerPos.z;
            double dist = std::sqrt(distX * distX + distZ * distZ);

            if (dist > unloadRadius) {
                toUnload.push_back(key);
            }
        }
    }

    for (uint64_t key : toUnload) {
        UnloadChunk(m_chunks[key].coord);
    }
}

bool WorldStreamer::RequestLoad(const ChunkCoord& chunk) {
    uint64_t key = MakeKey(chunk);
    auto it = m_chunks.find(key);

    if (it != m_chunks.end()) {
        if (it->second.state == ChunkState::Loaded || it->second.state == ChunkState::Loading) {
            return false; // already loaded or loading
        }
        if (it->second.state == ChunkState::Cached) {
            // Try to load from cache
            if (LoadChunkFromCache(chunk)) {
                return true;
            }
        }
    }

    // Create new entry in Loading state
    ChunkEntry entry;
    entry.coord = chunk;
    entry.state = ChunkState::Loading;
    m_chunks[key] = entry;
    return true;
}

void WorldStreamer::SetChunkData(const ChunkCoord& chunk, const std::vector<uint8_t>& data) {
    uint64_t key = MakeKey(chunk);
    auto it = m_chunks.find(key);
    if (it == m_chunks.end()) {
        ChunkEntry entry;
        entry.coord = chunk;
        m_chunks[key] = entry;
        it = m_chunks.find(key);
    }
    it->second.data = data;
    it->second.state = ChunkState::Loaded;
}

void WorldStreamer::UnloadChunk(const ChunkCoord& chunk) {
    uint64_t key = MakeKey(chunk);
    auto it = m_chunks.find(key);
    if (it == m_chunks.end()) return;

    // Try to cache to disk first
    if (!m_cacheDir.empty() && !it->second.data.empty()) {
        SaveChunkToCache(chunk);
        it->second.state = ChunkState::Cached;
        it->second.data.clear();
    } else {
        m_chunks.erase(it);
    }
}

ChunkState WorldStreamer::GetChunkState(const ChunkCoord& chunk) const {
    uint64_t key = MakeKey(chunk);
    auto it = m_chunks.find(key);
    if (it == m_chunks.end()) return ChunkState::Unloaded;
    return it->second.state;
}

std::vector<ChunkCoord> WorldStreamer::GetLoadedChunks() const {
    std::vector<ChunkCoord> result;
    for (const auto& [key, entry] : m_chunks) {
        if (entry.state == ChunkState::Loaded) {
            result.push_back(entry.coord);
        }
    }
    return result;
}

bool WorldStreamer::SaveChunkToCache(const ChunkCoord& chunk) const {
    if (m_cacheDir.empty()) return false;

    uint64_t key = MakeKey(chunk);
    auto it = m_chunks.find(key);
    if (it == m_chunks.end() || it->second.data.empty()) return false;

    std::filesystem::create_directories(m_cacheDir);

    std::ofstream file(CacheFilePath(chunk), std::ios::binary);
    if (!file.is_open()) return false;

    file.write(reinterpret_cast<const char*>(it->second.data.data()),
               static_cast<std::streamsize>(it->second.data.size()));
    return file.good();
}

bool WorldStreamer::LoadChunkFromCache(const ChunkCoord& chunk) {
    if (m_cacheDir.empty()) return false;

    std::string path = CacheFilePath(chunk);
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    auto size = file.tellg();
    if (size <= 0) return false;
    file.seekg(0);

    std::vector<uint8_t> data(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(data.data()), size);

    if (!file.good()) return false;

    uint64_t key = MakeKey(chunk);
    auto& entry = m_chunks[key];
    entry.coord = chunk;
    entry.data = std::move(data);
    entry.state = ChunkState::Loaded;
    return true;
}

size_t WorldStreamer::LoadedCount() const {
    size_t count = 0;
    for (const auto& [key, entry] : m_chunks) {
        if (entry.state == ChunkState::Loaded) ++count;
    }
    return count;
}

size_t WorldStreamer::CachedCount() const {
    size_t count = 0;
    for (const auto& [key, entry] : m_chunks) {
        if (entry.state == ChunkState::Cached) ++count;
    }
    return count;
}

}
