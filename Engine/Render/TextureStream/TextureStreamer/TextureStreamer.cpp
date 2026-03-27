#include "Engine/Render/TextureStream/TextureStreamer/TextureStreamer.h"
#include <algorithm>
#include <list>
#include <unordered_map>
#include <vector>

namespace Engine {

struct TexEntry {
    uint32_t id;
    uint32_t mipCount{1};
    std::vector<uint64_t> bytesPerMip;
    uint32_t residentMip{0};  // 0 = full res loaded (highest mip = lowest res)
    uint32_t requestedMip{0};
    float    urgency{1.f};
    bool     pending{false};
};

struct TextureStreamer::Impl {
    uint64_t budgetBytes{256*1024*1024ULL};
    uint64_t usedBytes{0};
    float    tickAccum{0};
    std::unordered_map<uint32_t,TexEntry> textures;
    std::function<void(uint32_t,uint32_t)> onLoaded;
    std::function<void(uint32_t,uint32_t)> onEvicted;

    TexEntry* Find(uint32_t id){ auto it=textures.find(id); return it!=textures.end()?&it->second:nullptr; }

    void Evict(){
        // LRU-evict: drop entries with highest mip index (lowest res) to free budget
        while(usedBytes>budgetBytes){
            uint32_t worst=0; float lowestUrgency=1e30f;
            for(auto& [id,e]:textures){
                if(e.residentMip>0&&e.urgency<lowestUrgency){
                    lowestUrgency=e.urgency; worst=id;
                }
            }
            if(!worst) break;
            auto& e=textures[worst];
            uint32_t prevMip=e.residentMip;
            usedBytes-=(prevMip<e.bytesPerMip.size())?e.bytesPerMip[prevMip]:0;
            e.residentMip++;
            if(onEvicted) onEvicted(worst,prevMip);
        }
    }

    void LoadMip(TexEntry& e, uint32_t targetMip){
        if(e.residentMip<=targetMip) return; // already equal or higher res
        uint64_t addBytes=(targetMip<e.bytesPerMip.size())?e.bytesPerMip[targetMip]:0;
        // Release higher mip bytes
        uint64_t freeBytes=(e.residentMip<e.bytesPerMip.size())?e.bytesPerMip[e.residentMip]:0;
        usedBytes+=addBytes;
        usedBytes=(freeBytes<=usedBytes)?usedBytes-freeBytes:0;
        e.residentMip=targetMip;
        e.pending=false;
        if(onLoaded) onLoaded(e.id,targetMip);
        Evict();
    }
};

TextureStreamer::TextureStreamer(): m_impl(new Impl){}
TextureStreamer::~TextureStreamer(){ Shutdown(); delete m_impl; }

void TextureStreamer::Init(uint64_t budget){ m_impl->budgetBytes=budget; }
void TextureStreamer::Shutdown(){ m_impl->textures.clear(); m_impl->usedBytes=0; }
void TextureStreamer::Reset(){ Shutdown(); }

void TextureStreamer::RegisterTexture(uint32_t id, uint32_t mipCount, const uint64_t* bytesPerMip){
    TexEntry e; e.id=id; e.mipCount=mipCount;
    e.bytesPerMip.assign(bytesPerMip, bytesPerMip+mipCount);
    e.residentMip=mipCount-1; // start with lowest res
    m_impl->textures[id]=e;
}
void TextureStreamer::UnregisterTexture(uint32_t id){
    auto* e=m_impl->Find(id);
    if(e) m_impl->usedBytes-=(e->residentMip<e->bytesPerMip.size())?e->bytesPerMip[e->residentMip]:0;
    m_impl->textures.erase(id);
}

void TextureStreamer::SetResidency(uint32_t id, uint32_t mip){
    auto* e=m_impl->Find(id); if(e){ e->requestedMip=mip; e->pending=true; }
}
void TextureStreamer::ForceResident(uint32_t id, uint32_t mip){
    auto* e=m_impl->Find(id); if(e) m_impl->LoadMip(*e,mip);
}

void TextureStreamer::Update(TSVec3 /*camPos*/, const float* /*frustumPlanes*/){
    // Simple stub: mark all textures with urgency-adjusted residency requests
    for(auto& [id,e]:m_impl->textures){
        if(!e.pending) e.requestedMip=e.mipCount/2; // default mid-mip
    }
}

void TextureStreamer::Tick(float dt){
    m_impl->tickAccum+=dt;
    if(m_impl->tickAccum<0.016f) return;
    m_impl->tickAccum=0;
    // Process one pending load per tick
    for(auto& [id,e]:m_impl->textures){
        if(e.pending){ m_impl->LoadMip(e,e.requestedMip); break; }
    }
}

uint32_t TextureStreamer::GetResidentMip(uint32_t id) const {
    auto* e=m_impl->Find(id); return e?e->residentMip:0;
}
uint64_t TextureStreamer::GetUsedBytes()   const { return m_impl->usedBytes; }
uint64_t TextureStreamer::GetBudgetBytes() const { return m_impl->budgetBytes; }
uint32_t TextureStreamer::GetPendingCount() const {
    uint32_t c=0; for(auto& [id,e]:m_impl->textures) if(e.pending) c++; return c;
}
void TextureStreamer::SetBudgetBytes(uint64_t b){ m_impl->budgetBytes=b; }
void TextureStreamer::SetUrgencyBias(uint32_t id, float bias){
    auto* e=m_impl->Find(id); if(e) e->urgency=bias;
}
void TextureStreamer::SetOnMipLoaded (std::function<void(uint32_t,uint32_t)> cb){ m_impl->onLoaded=cb; }
void TextureStreamer::SetOnMipEvicted(std::function<void(uint32_t,uint32_t)> cb){ m_impl->onEvicted=cb; }

} // namespace Engine
