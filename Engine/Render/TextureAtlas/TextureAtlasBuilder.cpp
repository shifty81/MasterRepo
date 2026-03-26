#include "Engine/Render/TextureAtlas/TextureAtlasBuilder.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>

namespace Engine {

struct ShelfPacker {
    uint32_t atlasW{0}, atlasH{0}, padding{1};
    uint32_t curX{0}, curY{0}, shelfH{0};

    bool Pack(uint32_t w, uint32_t h, uint32_t& outX, uint32_t& outY) {
        uint32_t pw=w+padding, ph=h+padding;
        if(curX+pw > atlasW) { curY+=shelfH; curX=0; shelfH=0; }
        if(curY+ph > atlasH) return false;
        outX=curX; outY=curY;
        curX+=pw; shelfH=std::max(shelfH,ph);
        return true;
    }
};

struct TextureAtlasBuilder::Impl {
    uint32_t             atlasW{2048}, atlasH{2048}, padding{1};
    std::vector<AtlasEntry> entries;
    std::vector<uint8_t>    pixels;

    AtlasEntry* Find(const std::string& name){
        for(auto& e:entries) if(e.name==name) return &e;
        return nullptr;
    }
};

TextureAtlasBuilder::TextureAtlasBuilder() : m_impl(new Impl()) {}
TextureAtlasBuilder::~TextureAtlasBuilder() { delete m_impl; }

void TextureAtlasBuilder::Init(uint32_t w, uint32_t h, uint32_t pad){
    m_impl->atlasW=w; m_impl->atlasH=h; m_impl->padding=pad;
    m_impl->pixels.assign((size_t)w*h*4, 0);
}
void TextureAtlasBuilder::Clear(){ m_impl->entries.clear(); m_impl->pixels.assign((size_t)m_impl->atlasW*m_impl->atlasH*4,0); }

bool TextureAtlasBuilder::AddFromFile(const std::string& name, const std::string&) {
    // Stub: in production this would load via stb_image etc.
    // Add a 16×16 placeholder
    std::vector<uint8_t> rgba(16*16*4, 255);
    return AddFromBuffer(name, rgba, 16, 16);
}
bool TextureAtlasBuilder::AddFromBuffer(const std::string& name,
                                         const std::vector<uint8_t>& rgba,
                                         uint32_t w, uint32_t h) {
    if(m_impl->Find(name)) return false; // already added
    AtlasEntry e; e.name=name; e.rgba=rgba; e.width=w; e.height=h;
    m_impl->entries.push_back(std::move(e));
    return true;
}

bool TextureAtlasBuilder::Build() {
    // Sort largest-first for better packing
    std::sort(m_impl->entries.begin(),m_impl->entries.end(),[](auto& a,auto& b){ return a.height>b.height; });
    ShelfPacker packer; packer.atlasW=m_impl->atlasW; packer.atlasH=m_impl->atlasH; packer.padding=m_impl->padding;
    m_impl->pixels.assign((size_t)m_impl->atlasW*m_impl->atlasH*4, 0);
    for(auto& e:m_impl->entries) {
        uint32_t px,py;
        if(!packer.Pack(e.width,e.height,px,py)) return false;
        e.uv.pixelX=px; e.uv.pixelY=py; e.uv.pixelW=e.width; e.uv.pixelH=e.height;
        e.uv.u=(float)px/(float)m_impl->atlasW; e.uv.v=(float)py/(float)m_impl->atlasH;
        e.uv.w=(float)e.width/(float)m_impl->atlasW; e.uv.h=(float)e.height/(float)m_impl->atlasH;
        // Blit pixels
        for(uint32_t row=0;row<e.height;++row) {
            uint32_t dstOff=((py+row)*m_impl->atlasW+px)*4;
            uint32_t srcOff=row*e.width*4;
            if(dstOff+(e.width*4)>(size_t)m_impl->atlasW*m_impl->atlasH*4) break;
            if(srcOff+(e.width*4)<=e.rgba.size())
                std::copy(e.rgba.begin()+srcOff,e.rgba.begin()+srcOff+e.width*4,m_impl->pixels.begin()+dstOff);
        }
        e.packed=true;
    }
    return true;
}

bool TextureAtlasBuilder::HasEntry(const std::string& name) const { return m_impl->Find(name)!=nullptr; }
UVRect TextureAtlasBuilder::GetUV(const std::string& name) const {
    auto* e=m_impl->Find(name); return e?e->uv:UVRect{};
}
std::unordered_map<std::string,UVRect> TextureAtlasBuilder::AllUVs() const {
    std::unordered_map<std::string,UVRect> out;
    for(auto& e:m_impl->entries) out[e.name]=e.uv;
    return out;
}

const std::vector<uint8_t>& TextureAtlasBuilder::GetAtlasPixels() const { return m_impl->pixels; }
uint32_t TextureAtlasBuilder::GetAtlasWidth()  const { return m_impl->atlasW; }
uint32_t TextureAtlasBuilder::GetAtlasHeight() const { return m_impl->atlasH; }

bool TextureAtlasBuilder::ExportAtlas(const std::string& path) const {
    std::ofstream f(path,std::ios::binary); if(!f.is_open()) return false;
    // Write PPM binary
    std::ostringstream hdr; hdr<<"P6\n"<<m_impl->atlasW<<" "<<m_impl->atlasH<<"\n255\n";
    f<<hdr.str();
    for(size_t i=0;i<(size_t)m_impl->atlasW*m_impl->atlasH;++i)
        f.write((char*)m_impl->pixels.data()+i*4,3);
    return true;
}
bool TextureAtlasBuilder::ExportMap(const std::string& path) const {
    std::ofstream f(path); if(!f.is_open()) return false;
    f<<"{\n";
    bool first=true;
    for(auto& e:m_impl->entries) {
        if(!first) f<<",\n"; first=false;
        f<<"  \""<<e.name<<"\":{\"u\":"<<e.uv.u<<",\"v\":"<<e.uv.v
          <<",\"w\":"<<e.uv.w<<",\"h\":"<<e.uv.h<<"}";
    }
    f<<"\n}\n"; return true;
}

} // namespace Engine
