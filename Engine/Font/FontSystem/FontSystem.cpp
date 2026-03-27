#include "Engine/Font/FontSystem/FontSystem.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Engine {

// ── UTF-8 utilities ──────────────────────────────────────────────────────────

uint32_t FontSystem::DecodeUTF8Codepoint(const char* str, uint32_t& byteLen)
{
    unsigned char c = (unsigned char)*str;
    if (!c)             { byteLen=0; return 0; }
    if (c < 0x80)       { byteLen=1; return c; }
    if ((c & 0xE0)==0xC0) {
        byteLen=2;
        return ((c&0x1F)<<6)  | ((unsigned char)str[1]&0x3F);
    }
    if ((c & 0xF0)==0xE0) {
        byteLen=3;
        return ((c&0x0F)<<12) | (((unsigned char)str[1]&0x3F)<<6) | ((unsigned char)str[2]&0x3F);
    }
    byteLen=4;
    return ((c&0x07)<<18) | (((unsigned char)str[1]&0x3F)<<12) |
           (((unsigned char)str[2]&0x3F)<<6) | ((unsigned char)str[3]&0x3F);
}

std::vector<uint32_t> FontSystem::UTF8ToCodepoints(const std::string& utf8)
{
    std::vector<uint32_t> cps;
    const char* p = utf8.c_str();
    while (*p) {
        uint32_t bl=0;
        uint32_t cp = DecodeUTF8Codepoint(p, bl);
        if (!bl) break;
        cps.push_back(cp); p+=bl;
    }
    return cps;
}

// ── Impl ─────────────────────────────────────────────────────────────────────

struct AtlasEntry {
    uint32_t    ptSize{0};
    uint32_t    width{512}, height{512};
    std::vector<uint8_t> data;  ///< grayscale bitmap (simulated)
    std::unordered_map<uint32_t, GlyphInfo> glyphs;
};

struct FaceEntry {
    uint32_t  faceId{0};
    std::string path;
    std::vector<AtlasEntry> atlases;
    uint32_t  fallbackFaceId{0};
    AtlasMode mode{AtlasMode::Bitmap};
};

static GlyphInfo MakeGlyph(uint32_t cp, uint32_t ptSize) {
    GlyphInfo g;
    g.codepoint = cp;
    g.valid     = true;
    g.width     = (int32_t)(ptSize * 0.6f);
    g.height    = (int32_t)(ptSize);
    g.bearingX  = 1;
    g.bearingY  = (int32_t)(ptSize * 0.8f);
    g.advance   = g.width + 2;
    // Fake atlas UV
    float u = (float)(cp % 32) / 32.f;
    float v = (float)(cp / 32 % 32) / 32.f;
    g.u0=u; g.v0=v; g.u1=u+1.f/32.f; g.v1=v+1.f/32.f;
    return g;
}

static const GlyphInfo s_invalid{};

struct FontSystem::Impl {
    std::vector<FaceEntry>      faces;
    uint32_t                    nextFaceId{1};
    std::function<void(uint32_t,uint32_t)> onAtlasRebuilt;

    FaceEntry* FindFace(uint32_t id) {
        for (auto& f : faces) if (f.faceId==id) return &f;
        return nullptr;
    }
    AtlasEntry* FindAtlas(FaceEntry& fe, uint32_t ptSize) {
        for (auto& a : fe.atlases) if (a.ptSize==ptSize) return &a;
        return nullptr;
    }
    AtlasEntry& EnsureAtlas(FaceEntry& fe, uint32_t ptSize) {
        auto* a = FindAtlas(fe, ptSize);
        if (a) return *a;
        AtlasEntry ae;
        ae.ptSize=ptSize; ae.data.resize(ae.width*ae.height, 0);
        fe.atlases.push_back(ae);
        return fe.atlases.back();
    }
    const GlyphInfo& GetOrMake(uint32_t faceId, uint32_t ptSize, uint32_t cp) {
        auto* fe = FindFace(faceId);
        if (!fe) return s_invalid;
        auto& ae = EnsureAtlas(*fe, ptSize);
        auto it  = ae.glyphs.find(cp);
        if (it != ae.glyphs.end()) return it->second;
        ae.glyphs[cp] = MakeGlyph(cp, ptSize);
        return ae.glyphs[cp];
    }
};

FontSystem::FontSystem()  : m_impl(new Impl) {}
FontSystem::~FontSystem() { Shutdown(); delete m_impl; }

void FontSystem::Init()     {}
void FontSystem::Shutdown() { m_impl->faces.clear(); }

uint32_t FontSystem::LoadFace(const FontFaceDesc& desc)
{
    FaceEntry fe;
    fe.faceId = m_impl->nextFaceId++;
    fe.path   = desc.path;
    fe.mode   = desc.mode;
    for (auto sz : desc.sizes) {
        auto& ae = m_impl->EnsureAtlas(fe, sz);
        // Pre-rasterise basic ASCII
        for (uint32_t cp=32; cp<128; cp++) {
            ae.glyphs[cp] = MakeGlyph(cp, sz);
        }
    }
    m_impl->faces.push_back(fe);
    return fe.faceId;
}

uint32_t FontSystem::LoadFace(const std::string& path, std::vector<uint32_t> sizes)
{
    FontFaceDesc d; d.path=path; d.sizes=sizes;
    return LoadFace(d);
}

void FontSystem::UnloadFace(uint32_t id)
{
    auto& v = m_impl->faces;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& f){ return f.faceId==id; }), v.end());
}

bool FontSystem::HasFace(uint32_t id) const
{
    return m_impl->FindFace(id) != nullptr;
}

void FontSystem::SetFallback(uint32_t faceId, uint32_t fallbackId)
{
    auto* f = m_impl->FindFace(faceId);
    if (f) f->fallbackFaceId = fallbackId;
}

const GlyphInfo& FontSystem::GetGlyph(uint32_t faceId, uint32_t ptSize, uint32_t cp) const
{
    return const_cast<FontSystem::Impl*>(m_impl)->GetOrMake(faceId, ptSize, cp);
}

bool FontSystem::HasGlyph(uint32_t faceId, uint32_t ptSize, uint32_t cp) const
{
    auto* fe = m_impl->FindFace(faceId);
    if (!fe) return false;
    auto* ae = m_impl->FindAtlas(*fe, ptSize);
    return ae && ae->glyphs.count(cp) > 0;
}

TextMetrics FontSystem::MeasureText(uint32_t faceId, uint32_t ptSize,
                                     const std::string& utf8, float wrapWidth) const
{
    auto cps = UTF8ToCodepoints(utf8);
    TextMetrics m;
    float lineW=0.f; m.lineCount=1;
    for (auto cp : cps) {
        if (cp=='\n') { m.width=std::max(m.width,lineW); lineW=0.f; m.lineCount++; continue; }
        const auto& g = const_cast<FontSystem::Impl*>(m_impl)->GetOrMake(faceId, ptSize, cp);
        lineW += (float)g.advance;
        if (wrapWidth>0.f && lineW>wrapWidth) {
            m.width=std::max(m.width,wrapWidth); lineW=(float)g.advance; m.lineCount++;
        }
    }
    m.width  = std::max(m.width, lineW);
    m.height = (float)(ptSize * m.lineCount);
    return m;
}

std::string FontSystem::WrapText(uint32_t faceId, uint32_t ptSize,
                                  const std::string& utf8, float wrapWidth) const
{
    auto cps = UTF8ToCodepoints(utf8);
    std::string out; float lineW=0.f;
    for (auto cp : cps) {
        const auto& g = const_cast<FontSystem::Impl*>(m_impl)->GetOrMake(faceId, ptSize, cp);
        if (lineW + g.advance > wrapWidth && lineW > 0.f) { out+='\n'; lineW=0.f; }
        out += (char)(cp<128?cp:'?');
        lineW += (float)g.advance;
    }
    return out;
}

std::string FontSystem::TruncateText(uint32_t faceId, uint32_t ptSize,
                                      const std::string& utf8, float maxWidth,
                                      const std::string& ellipsis) const
{
    auto dots = MeasureText(faceId, ptSize, ellipsis);
    auto cps  = UTF8ToCodepoints(utf8);
    std::string out; float w=0.f;
    for (auto cp : cps) {
        const auto& g = const_cast<FontSystem::Impl*>(m_impl)->GetOrMake(faceId, ptSize, cp);
        if (w + g.advance + dots.width > maxWidth) { out+=ellipsis; return out; }
        out += (char)(cp<128?cp:'?'); w += (float)g.advance;
    }
    return out;
}

const uint8_t* FontSystem::GetAtlasData(uint32_t faceId, uint32_t ptSize) const
{
    auto* fe = m_impl->FindFace(faceId);
    if (!fe) return nullptr;
    auto* ae = m_impl->FindAtlas(*fe, ptSize);
    return ae ? ae->data.data() : nullptr;
}

uint32_t FontSystem::GetAtlasWidth(uint32_t faceId, uint32_t ptSize) const
{
    auto* fe = m_impl->FindFace(faceId);
    if (!fe) return 0;
    auto* ae = m_impl->FindAtlas(*fe, ptSize);
    return ae ? ae->width : 0;
}

uint32_t FontSystem::GetAtlasHeight(uint32_t faceId, uint32_t ptSize) const
{
    auto* fe = m_impl->FindFace(faceId);
    if (!fe) return 0;
    auto* ae = m_impl->FindAtlas(*fe, ptSize);
    return ae ? ae->height : 0;
}

uint32_t FontSystem::GetAtlasChannels() const { return 1; }

void FontSystem::SetOnAtlasRebuilt(std::function<void(uint32_t,uint32_t)> cb)
{
    m_impl->onAtlasRebuilt = cb;
}

} // namespace Engine
