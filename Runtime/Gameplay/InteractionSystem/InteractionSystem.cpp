#include "Runtime/Gameplay/InteractionSystem/InteractionSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct Interactor {
    uint32_t id{0};
    float    pos[3]{};
    float    range{3.f};
};

struct InteractionSystem::Impl {
    std::unordered_map<uint32_t, InteractableState> objects;
    std::unordered_map<uint32_t, Interactor>        interactors;
    uint32_t nextHandle{1};
    std::function<void(uint32_t,uint32_t)> onInteract;

    float Dist(const float a[3], const float b[3]) const {
        float dx=a[0]-b[0],dy=a[1]-b[1],dz=a[2]-b[2];
        return std::sqrt(dx*dx+dy*dy+dz*dz);
    }

    std::vector<InteractableState*> Candidates(const Interactor& actor) {
        std::vector<InteractableState*> out;
        for(auto& [h,s]: objects) {
            if(!s.alive || !s.desc.enabled || s.cooldownRemaining>0.f) continue;
            float d=Dist(actor.pos, s.desc.position);
            float r=std::min(actor.range, s.desc.range);
            if(d <= r) out.push_back(&s);
        }
        std::sort(out.begin(),out.end(),[&](auto* a, auto* b){
            if(a->desc.priority!=b->desc.priority) return a->desc.priority>b->desc.priority;
            return Dist(actor.pos,a->desc.position)<Dist(actor.pos,b->desc.position);
        });
        return out;
    }
};

InteractionSystem::InteractionSystem() : m_impl(new Impl()) {}
InteractionSystem::~InteractionSystem() { delete m_impl; }
void InteractionSystem::Init()     {}
void InteractionSystem::Shutdown() { m_impl->objects.clear(); }

uint32_t InteractionSystem::Register(const InteractableDesc& desc) {
    uint32_t h=m_impl->nextHandle++;
    InteractableState& s=m_impl->objects[h];
    s.handle=h; s.desc=desc; s.alive=true;
    return h;
}
void InteractionSystem::Unregister(uint32_t h){ m_impl->objects.erase(h); }
bool InteractionSystem::HasInteractable(uint32_t h) const { return m_impl->objects.count(h)>0; }
void InteractionSystem::SetEnabled(uint32_t h, bool e){
    auto it=m_impl->objects.find(h); if(it!=m_impl->objects.end()) it->second.desc.enabled=e;
}
void InteractionSystem::SetPosition(uint32_t h, const float pos[3]){
    auto it=m_impl->objects.find(h); if(it!=m_impl->objects.end()) for(int i=0;i<3;++i) it->second.desc.position[i]=pos[i];
}
InteractableState InteractionSystem::GetState(uint32_t h) const {
    auto it=m_impl->objects.find(h); return it!=m_impl->objects.end()?it->second:InteractableState{};
}

void InteractionSystem::SetInteractorPosition(uint32_t id, const float p[3]) {
    auto& a=m_impl->interactors[id]; a.id=id; for(int i=0;i<3;++i) a.pos[i]=p[i];
}
void InteractionSystem::SetInteractorRange(uint32_t id, float r) {
    m_impl->interactors[id].range=r;
}

bool InteractionSystem::HasCandidate(uint32_t id) const {
    auto it=m_impl->interactors.find(id); if(it==m_impl->interactors.end()) return false;
    return !const_cast<Impl*>(m_impl)->Candidates(it->second).empty();
}
uint32_t InteractionSystem::GetBestCandidateHandle(uint32_t id) const {
    auto it=m_impl->interactors.find(id); if(it==m_impl->interactors.end()) return 0;
    auto cands=const_cast<Impl*>(m_impl)->Candidates(it->second);
    return cands.empty()?0:cands.front()->handle;
}
InteractableDesc InteractionSystem::GetBestCandidate(uint32_t id) const {
    uint32_t h=GetBestCandidateHandle(id);
    auto it=m_impl->objects.find(h); return it!=m_impl->objects.end()?it->second.desc:InteractableDesc{};
}
std::vector<InteractableState> InteractionSystem::GetCandidates(uint32_t id) const {
    auto it=m_impl->interactors.find(id); if(it==m_impl->interactors.end()) return {};
    std::vector<InteractableState> out;
    for(auto* s: const_cast<Impl*>(m_impl)->Candidates(it->second)) out.push_back(*s);
    return out;
}

bool InteractionSystem::Interact(uint32_t interactorId) {
    uint32_t h=GetBestCandidateHandle(interactorId);
    return h ? InteractWith(interactorId,h) : false;
}
bool InteractionSystem::InteractWith(uint32_t interactorId, uint32_t h) {
    auto it=m_impl->objects.find(h); if(it==m_impl->objects.end()) return false;
    auto& s=it->second;
    if(!s.alive||!s.desc.enabled||s.cooldownRemaining>0.f) return false;
    s.useCount++;
    s.cooldownRemaining=s.desc.cooldown;
    if(s.desc.onInteract) s.desc.onInteract(interactorId);
    if(m_impl->onInteract) m_impl->onInteract(interactorId,h);
    if(s.desc.oneShot) s.alive=false;
    return true;
}

void InteractionSystem::Update(float dt) {
    for(auto& [h,s]: m_impl->objects) {
        if(s.cooldownRemaining>0.f) s.cooldownRemaining=std::max(0.f,s.cooldownRemaining-dt);
    }
}
void InteractionSystem::OnInteract(std::function<void(uint32_t,uint32_t)> cb){ m_impl->onInteract=std::move(cb); }

} // namespace Runtime
