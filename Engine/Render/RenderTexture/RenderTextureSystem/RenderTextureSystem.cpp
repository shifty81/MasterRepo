#include "Engine/Render/RenderTexture/RenderTextureSystem/RenderTextureSystem.h"
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace Engine {

static uint32_t ChannelsForFormat(RTPixelFormat fmt){
    switch(fmt){
        case RTPixelFormat::Depth24:
        case RTPixelFormat::Depth32F: return 1;
        default: return 4;
    }
}

struct RTTex {
    uint32_t      w{0}, h{0};
    RTPixelFormat format{RTPixelFormat::RGBA8};
    uint32_t      mips{1};
    bool          linear{true};
    std::vector<float> data;

    uint32_t PixelStride() const { return ChannelsForFormat(format); }
    uint32_t DataSize()    const { return w*h*PixelStride(); }
    void Alloc(){ data.assign(DataSize(),0.f); }
};

struct RenderTextureSystem::Impl {
    std::unordered_map<uint32_t,RTTex> textures;
    RTTex* Find(uint32_t id){ auto it=textures.find(id); return it!=textures.end()?&it->second:nullptr; }
};

RenderTextureSystem::RenderTextureSystem(): m_impl(new Impl){}
RenderTextureSystem::~RenderTextureSystem(){ Shutdown(); delete m_impl; }
void RenderTextureSystem::Init(){}
void RenderTextureSystem::Shutdown(){ m_impl->textures.clear(); }
void RenderTextureSystem::Reset(){ m_impl->textures.clear(); }

bool RenderTextureSystem::CreateTexture(uint32_t id,uint32_t w,uint32_t h,
                                         RTPixelFormat fmt,uint32_t mips){
    if(m_impl->textures.count(id)) return false;
    RTTex t; t.w=w; t.h=h; t.format=fmt; t.mips=mips;
    t.Alloc();
    m_impl->textures[id]=std::move(t);
    return true;
}
void RenderTextureSystem::DestroyTexture(uint32_t id){ m_impl->textures.erase(id); }
void RenderTextureSystem::Resize(uint32_t id,uint32_t w,uint32_t h){
    auto* t=m_impl->Find(id); if(!t) return;
    t->w=w; t->h=h; t->Alloc();
}
void RenderTextureSystem::Clear(uint32_t id,float r,float g,float b,float a){
    auto* t=m_impl->Find(id); if(!t) return;
    uint32_t ch=t->PixelStride();
    for(uint32_t i=0;i<t->w*t->h;i++){
        if(ch>=1) t->data[i*ch+0]=(ch==1?r:r);
        if(ch>=2) t->data[i*ch+1]=g;
        if(ch>=3) t->data[i*ch+2]=b;
        if(ch>=4) t->data[i*ch+3]=a;
    }
}
void RenderTextureSystem::Blit(uint32_t srcId,uint32_t dstId){
    auto* src=m_impl->Find(srcId); auto* dst=m_impl->Find(dstId);
    if(!src||!dst) return;
    uint32_t n=std::min(src->data.size(),dst->data.size());
    std::copy(src->data.begin(),src->data.begin()+n,dst->data.begin());
}
void RenderTextureSystem::BlitRegion(uint32_t srcId,uint32_t dstId,
    uint32_t sx,uint32_t sy,uint32_t sw,uint32_t sh,
    uint32_t dx,uint32_t dy,uint32_t dw,uint32_t dh){
    auto* src=m_impl->Find(srcId); auto* dst=m_impl->Find(dstId);
    if(!src||!dst) return;
    uint32_t ch=src->PixelStride();
    for(uint32_t py=0;py<dh;py++) for(uint32_t px=0;px<dw;px++){
        uint32_t spx=sx+(uint32_t)(px*(float)sw/dw);
        uint32_t spy=sy+(uint32_t)(py*(float)sh/dh);
        uint32_t dpx=dx+px, dpy=dy+py;
        if(spx>=src->w||spy>=src->h||dpx>=dst->w||dpy>=dst->h) continue;
        uint32_t si=(spy*src->w+spx)*ch, di=(dpy*dst->w+dpx)*ch;
        for(uint32_t c=0;c<ch;c++) if(si+c<src->data.size()&&di+c<dst->data.size())
            dst->data[di+c]=src->data[si+c];
    }
}
void RenderTextureSystem::GenerateMips(uint32_t){ /* stub - no-op for CPU */ }

bool RenderTextureSystem::ReadPixels(uint32_t id,uint32_t x,uint32_t y,
                                      uint32_t w,uint32_t h,
                                      std::vector<float>& out) const {
    auto* t=m_impl->Find(id); if(!t) return false;
    uint32_t ch=t->PixelStride();
    out.resize(w*h*ch);
    for(uint32_t py=0;py<h;py++) for(uint32_t px=0;px<w;px++){
        uint32_t sx=x+px, sy=y+py; if(sx>=t->w||sy>=t->h) continue;
        uint32_t si=(sy*t->w+sx)*ch, oi=(py*w+px)*ch;
        for(uint32_t c=0;c<ch;c++) out[oi+c]=t->data[si+c];
    }
    return true;
}
bool RenderTextureSystem::WritePixels(uint32_t id,uint32_t x,uint32_t y,
                                       uint32_t w,uint32_t h,
                                       const std::vector<float>& in){
    auto* t=m_impl->Find(id); if(!t) return false;
    uint32_t ch=t->PixelStride();
    for(uint32_t py=0;py<h;py++) for(uint32_t px=0;px<w;px++){
        uint32_t dx2=x+px, dy2=y+py; if(dx2>=t->w||dy2>=t->h) continue;
        uint32_t di=(dy2*t->w+dx2)*ch, ii=(py*w+px)*ch;
        for(uint32_t c=0;c<ch;c++) if(ii+c<in.size()) t->data[di+c]=in[ii+c];
    }
    return true;
}

uint32_t RenderTextureSystem::GetWidth(uint32_t id) const { auto* t=m_impl->Find(id);return t?t->w:0; }
uint32_t RenderTextureSystem::GetHeight(uint32_t id) const { auto* t=m_impl->Find(id);return t?t->h:0; }
RTPixelFormat RenderTextureSystem::GetFormat(uint32_t id) const { auto* t=m_impl->Find(id);return t?t->format:RTPixelFormat::RGBA8; }
uint32_t RenderTextureSystem::GetMipCount(uint32_t id) const { auto* t=m_impl->Find(id);return t?t->mips:0; }
void RenderTextureSystem::SetFilterMode(uint32_t id,bool linear){ auto* t=m_impl->Find(id);if(t)t->linear=linear; }

} // namespace Engine
