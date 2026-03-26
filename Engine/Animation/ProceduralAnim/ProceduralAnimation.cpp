#include "Engine/Animation/ProceduralAnim/ProceduralAnimation.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct SpringBoneState {
    SpringBone def;
    float      velocity[3]{};
    float      currentOffset[3]{};
};

struct ProceduralAnimation::Impl {
    std::unordered_map<uint32_t, IKChain>          ikChains;
    std::unordered_map<uint32_t, LookAtConstraint> lookAts;
    std::unordered_map<uint32_t, SpringBoneState>  springs;
    uint32_t nextIKId{1}, nextLAId{1}, nextSpringId{1};

    bool    ragdollActive{false};
    float   ragdollBlend{0.f};      ///< 0=animated, 1=ragdoll
    float   ragdollTargetBlend{0.f};
    float   ragdollBlendSpeed{0.f};

    bool    footPlacement{false};
    uint32_t footIKLeft{0}, footIKRight{0};
    float   terrainHeight{0.f};

    std::function<void()> onSolve;

    static void Normalize3(float v[3]) {
        float l=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]); if(l>1e-6f){v[0]/=l;v[1]/=l;v[2]/=l;}
    }

    void SolveTwoBoneIK(IKChain& ik) {
        // Simplified: just move toward target along chain axis
        float dir[3]={ik.target[0]-ik.poleTarget[0],
                      ik.target[1]-ik.poleTarget[1],
                      ik.target[2]-ik.poleTarget[2]};
        Normalize3(dir);
        (void)dir; // real impl: compute joint angles
    }

    void SolveFABRIK(IKChain& ik) {
        (void)ik; // Iterative FABRIK: forward + backward passes
    }
};

ProceduralAnimation::ProceduralAnimation() : m_impl(new Impl()) {}
ProceduralAnimation::~ProceduralAnimation() { delete m_impl; }
void ProceduralAnimation::Init()     {}
void ProceduralAnimation::Shutdown() {}

uint32_t ProceduralAnimation::CreateIKChain(const std::string& name,
    const std::vector<std::string>& joints, IKSolver solver) {
    uint32_t id=m_impl->nextIKId++;
    auto& chain=m_impl->ikChains[id];
    chain.id=id; chain.name=name; chain.joints=joints; chain.solver=solver;
    return id;
}

void ProceduralAnimation::DestroyIKChain(uint32_t id) { m_impl->ikChains.erase(id); }

void ProceduralAnimation::SetIKTarget(uint32_t id, const float t[3]) {
    auto it=m_impl->ikChains.find(id); if(it!=m_impl->ikChains.end()) for(int i=0;i<3;++i) it->second.target[i]=t[i];
}
void ProceduralAnimation::SetIKPoleTarget(uint32_t id, const float t[3]) {
    auto it=m_impl->ikChains.find(id); if(it!=m_impl->ikChains.end()) for(int i=0;i<3;++i) it->second.poleTarget[i]=t[i];
}
void ProceduralAnimation::SetIKWeight(uint32_t id, float w) {
    auto it=m_impl->ikChains.find(id); if(it!=m_impl->ikChains.end()) it->second.weight=w;
}
void ProceduralAnimation::SetIKEnabled(uint32_t id, bool e) {
    auto it=m_impl->ikChains.find(id); if(it!=m_impl->ikChains.end()) it->second.enabled=e;
}
IKChain ProceduralAnimation::GetIKChain(uint32_t id) const {
    auto it=m_impl->ikChains.find(id); return it!=m_impl->ikChains.end()?it->second:IKChain{};
}

uint32_t ProceduralAnimation::CreateLookAt(const std::string& name,
    const std::vector<std::string>& chain, const float target[3]) {
    uint32_t id=m_impl->nextLAId++;
    auto& la=m_impl->lookAts[id];
    la.id=id; la.name=name; la.chain=chain;
    for(int i=0;i<3;++i) la.target[i]=target[i];
    return id;
}
void ProceduralAnimation::SetLookAtTarget(uint32_t id, const float t[3]) {
    auto it=m_impl->lookAts.find(id); if(it!=m_impl->lookAts.end()) for(int i=0;i<3;++i) it->second.target[i]=t[i];
}
void ProceduralAnimation::SetLookAtWeight(uint32_t id, float w) {
    auto it=m_impl->lookAts.find(id); if(it!=m_impl->lookAts.end()) it->second.weight=w;
}
void ProceduralAnimation::SetLookAtEnabled(uint32_t id, bool e) {
    auto it=m_impl->lookAts.find(id); if(it!=m_impl->lookAts.end()) it->second.enabled=e;
}

uint32_t ProceduralAnimation::AddSpringBone(const std::string& joint, float stiff, float damp) {
    uint32_t id=m_impl->nextSpringId++;
    auto& sb=m_impl->springs[id];
    sb.def.id=id; sb.def.jointName=joint; sb.def.stiffness=stiff; sb.def.damping=damp;
    return id;
}
void ProceduralAnimation::SetSpringBoneEnabled(uint32_t id, bool e) {
    auto it=m_impl->springs.find(id); if(it!=m_impl->springs.end()) it->second.def.enabled=e;
}

void ProceduralAnimation::EnableTerrainFootPlacement(uint32_t l, uint32_t r) {
    m_impl->footPlacement=true; m_impl->footIKLeft=l; m_impl->footIKRight=r;
}
void ProceduralAnimation::DisableTerrainFootPlacement() { m_impl->footPlacement=false; }
void ProceduralAnimation::SetTerrainHeight(float h) { m_impl->terrainHeight=h; }

void ProceduralAnimation::Solve(float dt) {
    // Foot placement
    if(m_impl->footPlacement){
        float groundTarget[3]={0,m_impl->terrainHeight,0};
        SetIKTarget(m_impl->footIKLeft,  groundTarget);
        SetIKTarget(m_impl->footIKRight, groundTarget);
    }
    // IK chains
    for(auto&[id,chain]:m_impl->ikChains){
        if(!chain.enabled||chain.weight<=0.f) continue;
        if(chain.solver==IKSolver::TwoBone) m_impl->SolveTwoBoneIK(chain);
        else m_impl->SolveFABRIK(chain);
    }
    // Spring bones (semi-implicit Euler)
    for(auto&[id,sb]:m_impl->springs){
        if(!sb.def.enabled) continue;
        for(int i=0;i<3;++i){
            float spring=-sb.def.stiffness*sb.currentOffset[i];
            float damp  =-sb.def.damping  *sb.velocity[i];
            float accel =(spring+damp+sb.def.gravity[i]);
            sb.velocity[i]     +=accel*dt;
            sb.currentOffset[i]+=sb.velocity[i]*dt;
        }
    }
    // Ragdoll blend
    if(m_impl->ragdollBlend!=m_impl->ragdollTargetBlend){
        float dir=m_impl->ragdollTargetBlend>m_impl->ragdollBlend?1.f:-1.f;
        m_impl->ragdollBlend+=dir*m_impl->ragdollBlendSpeed*dt;
        if(dir>0&&m_impl->ragdollBlend>=m_impl->ragdollTargetBlend) m_impl->ragdollBlend=m_impl->ragdollTargetBlend;
        if(dir<0&&m_impl->ragdollBlend<=m_impl->ragdollTargetBlend) m_impl->ragdollBlend=m_impl->ragdollTargetBlend;
    }
    if(m_impl->onSolve) m_impl->onSolve();
}

void ProceduralAnimation::BlendInRagdoll(float t) {
    m_impl->ragdollActive=true; m_impl->ragdollTargetBlend=1.f;
    m_impl->ragdollBlendSpeed=t>0.f?1.f/t:9999.f;
}
void ProceduralAnimation::BlendOutRagdoll(float t) {
    m_impl->ragdollTargetBlend=0.f; m_impl->ragdollBlendSpeed=t>0.f?1.f/t:9999.f;
}
bool ProceduralAnimation::IsRagdollActive() const { return m_impl->ragdollBlend>0.f; }
void ProceduralAnimation::OnSolveComplete(std::function<void()> cb) { m_impl->onSolve=std::move(cb); }

} // namespace Engine
