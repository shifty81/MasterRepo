#include "Engine/Render/Atlas/TextureAtlas/TextureAtlas.h"
#include <unordered_map>

namespace Engine {

struct Rect { uint32_t x{}, y{}, w{}, h{}; };
struct ShelfAtlas {
    uint32_t width{}, height{};
    uint32_t shelfY{}, shelfH{}, shelfX{};
    std::unordered_map<uint32_t,Rect> sprites;
    std::function<void()> onFull;
};

struct TextureAtlas::Impl {
    std::unordered_map<uint32_t, ShelfAtlas> atlases;
};

TextureAtlas::TextureAtlas()  { m_impl = new Impl; }
TextureAtlas::~TextureAtlas() { delete m_impl; }
void TextureAtlas::Init    () {}
void TextureAtlas::Shutdown() { m_impl->atlases.clear(); }

bool TextureAtlas::Create(uint32_t id, uint32_t w, uint32_t h) {
    if (m_impl->atlases.count(id)) return false;
    ShelfAtlas a; a.width = w; a.height = h;
    m_impl->atlases[id] = a;
    return true;
}
void TextureAtlas::Destroy(uint32_t id) { m_impl->atlases.erase(id); }
void TextureAtlas::Reset  (uint32_t id) {
    auto it = m_impl->atlases.find(id); if (it == m_impl->atlases.end()) return;
    auto& a = it->second; a.sprites.clear(); a.shelfY = a.shelfH = a.shelfX = 0;
}

bool TextureAtlas::Pack(uint32_t atlasId, uint32_t spriteId, uint32_t sw, uint32_t sh) {
    auto it = m_impl->atlases.find(atlasId);
    if (it == m_impl->atlases.end()) return false;
    auto& a = it->second;
    // shelf-packing
    if (a.shelfX + sw > a.width) {
        a.shelfY += a.shelfH; a.shelfH = 0; a.shelfX = 0;
    }
    if (a.shelfY + sh > a.height) {
        if (a.onFull) a.onFull();
        return false;
    }
    Rect r; r.x = a.shelfX; r.y = a.shelfY; r.w = sw; r.h = sh;
    a.sprites[spriteId] = r;
    a.shelfX += sw;
    a.shelfH = (sh > a.shelfH) ? sh : a.shelfH;
    return true;
}
uint32_t TextureAtlas::PackBatch(uint32_t atlasId,
                                  const std::vector<uint32_t>& ids,
                                  const std::vector<uint32_t>& ws,
                                  const std::vector<uint32_t>& hs) {
    uint32_t n = (uint32_t)ids.size(); uint32_t packed = 0;
    for (uint32_t i = 0; i < n; ++i)
        if (Pack(atlasId, ids[i], ws[i], hs[i])) ++packed;
    return packed;
}
void TextureAtlas::Remove(uint32_t atlasId, uint32_t spriteId) {
    auto it = m_impl->atlases.find(atlasId);
    if (it != m_impl->atlases.end()) it->second.sprites.erase(spriteId);
}
bool TextureAtlas::GetUVRect(uint32_t atlasId, uint32_t spriteId,
                              float& u0, float& v0, float& u1, float& v1) const {
    auto it = m_impl->atlases.find(atlasId); if (it == m_impl->atlases.end()) return false;
    auto& a = it->second; auto it2 = a.sprites.find(spriteId); if (it2 == a.sprites.end()) return false;
    auto& r = it2->second;
    u0 = (float)r.x / a.width;  v0 = (float)r.y / a.height;
    u1 = (float)(r.x+r.w) / a.width; v1 = (float)(r.y+r.h) / a.height;
    return true;
}
bool TextureAtlas::GetPixelRect(uint32_t atlasId, uint32_t spriteId,
                                 uint32_t& x, uint32_t& y, uint32_t& w, uint32_t& h) const {
    auto it = m_impl->atlases.find(atlasId); if (it == m_impl->atlases.end()) return false;
    auto it2 = it->second.sprites.find(spriteId); if (it2 == it->second.sprites.end()) return false;
    x = it2->second.x; y = it2->second.y; w = it2->second.w; h = it2->second.h; return true;
}
void TextureAtlas::GetAtlasSize(uint32_t atlasId, uint32_t& w, uint32_t& h) const {
    auto it = m_impl->atlases.find(atlasId);
    w = it != m_impl->atlases.end() ? it->second.width  : 0;
    h = it != m_impl->atlases.end() ? it->second.height : 0;
}
float TextureAtlas::GetUsedArea(uint32_t atlasId) const {
    auto it = m_impl->atlases.find(atlasId); if (it == m_impl->atlases.end()) return 0;
    auto& a = it->second; float total = (float)(a.width * a.height); if (total <= 0) return 0;
    float used = 0; for (auto& [id, r] : a.sprites) used += r.w * r.h;
    return used / total;
}
uint32_t TextureAtlas::GetSpriteCount(uint32_t atlasId) const {
    auto it = m_impl->atlases.find(atlasId); return it != m_impl->atlases.end() ? (uint32_t)it->second.sprites.size() : 0;
}
uint32_t TextureAtlas::GetAllSprites(uint32_t atlasId, std::vector<uint32_t>& out) const {
    auto it = m_impl->atlases.find(atlasId); if (it == m_impl->atlases.end()) return 0;
    for (auto& [id,_] : it->second.sprites) out.push_back(id);
    return (uint32_t)out.size();
}
void TextureAtlas::SetOnFull(uint32_t atlasId, std::function<void()> cb) {
    auto it = m_impl->atlases.find(atlasId); if (it != m_impl->atlases.end()) it->second.onFull = std::move(cb);
}

} // namespace Engine
