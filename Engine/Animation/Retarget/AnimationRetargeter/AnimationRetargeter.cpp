#include "Engine/Animation/Retarget/AnimationRetargeter/AnimationRetargeter.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace Engine {

static void NLerpQ(const float a[4], const float b[4], float t, float out[4]){
    float d=a[0]*b[0]+a[1]*b[1]+a[2]*b[2]+a[3]*b[3];
    float sign=d<0.f?-1.f:1.f;
    for(int i=0;i<4;i++) out[i]=a[i]+(b[i]*sign-a[i])*t;
    float l=std::sqrt(out[0]*out[0]+out[1]*out[1]+out[2]*out[2]+out[3]*out[3])+1e-9f;
    for(int i=0;i<4;i++) out[i]/=l;
}

struct AnimationRetargeter::Impl {
    std::unordered_map<std::string,std::string> mapping; // srcBone → dstBone
    bool lengthScaling{true};
    float rootMotionScale{1.f};
    bool keepBindPose{true};
};

AnimationRetargeter::AnimationRetargeter()  : m_impl(new Impl){}
AnimationRetargeter::~AnimationRetargeter() { Shutdown(); delete m_impl; }
void AnimationRetargeter::Init()     {}
void AnimationRetargeter::Shutdown() { m_impl->mapping.clear(); }

void AnimationRetargeter::BuildMap(const RetargetSkeleton& src, const RetargetSkeleton& dst){
    m_impl->mapping.clear();
    for(uint32_t i=0;i<src.boneCount;i++){
        for(uint32_t j=0;j<dst.boneCount;j++){
            if(src.boneNames[i]==dst.boneNames[j]){
                m_impl->mapping[src.boneNames[i]]=dst.boneNames[j]; break;
            }
        }
    }
}
void AnimationRetargeter::SetCustomMapping(const std::string& s, const std::string& d){
    m_impl->mapping[s]=d;
}
void AnimationRetargeter::RemoveMapping(const std::string& s){ m_impl->mapping.erase(s); }
void AnimationRetargeter::ClearMappings(){ m_impl->mapping.clear(); }

void AnimationRetargeter::RetargetPose(const RetargetBone* srcPose,
                                         const RetargetSkeleton& srcSkel,
                                         const RetargetSkeleton& dstSkel,
                                         RetargetBone* outDstPose) const
{
    // Initialize with bind-like identity
    for(uint32_t di=0;di<dstSkel.boneCount;di++){
        outDstPose[di].position[0]=outDstPose[di].position[1]=outDstPose[di].position[2]=0;
        outDstPose[di].orientation[0]=outDstPose[di].orientation[1]=outDstPose[di].orientation[2]=0;
        outDstPose[di].orientation[3]=1;
        outDstPose[di].scale[0]=outDstPose[di].scale[1]=outDstPose[di].scale[2]=1;
    }
    for(uint32_t si=0;si<srcSkel.boneCount;si++){
        auto it=m_impl->mapping.find(srcSkel.boneNames[si]);
        if(it==m_impl->mapping.end()) continue;
        const std::string& dstName=it->second;
        // Find dst bone index
        for(uint32_t di=0;di<dstSkel.boneCount;di++){
            if(dstSkel.boneNames[di]!=dstName) continue;
            const RetargetBone& sb=srcPose[si];
            RetargetBone& db=outDstPose[di];
            // Copy orientation
            for(int k=0;k<4;k++) db.orientation[k]=sb.orientation[k];
            // Length scale: scale position by dst/src bone length ratio
            float ratio=1.f;
            if(m_impl->lengthScaling&&srcSkel.bindLength[si]>0.f)
                ratio=dstSkel.bindLength[di]/srcSkel.bindLength[si];
            for(int k=0;k<3;k++) db.position[k]=sb.position[k]*ratio;
            for(int k=0;k<3;k++) db.scale[k]=sb.scale[k];
            // Root motion
            if(si==0) for(int k=0;k<3;k++) db.position[k]*=m_impl->rootMotionScale;
            break;
        }
    }
}

void AnimationRetargeter::LerpPoses(const RetargetBone* a, const RetargetBone* b,
                                      uint32_t n, float t, RetargetBone* out) const
{
    for(uint32_t i=0;i<n;i++){
        for(int k=0;k<3;k++) out[i].position[k]=a[i].position[k]+(b[i].position[k]-a[i].position[k])*t;
        NLerpQ(a[i].orientation, b[i].orientation, t, out[i].orientation);
        for(int k=0;k<3;k++) out[i].scale[k]=a[i].scale[k]+(b[i].scale[k]-a[i].scale[k])*t;
    }
}

void AnimationRetargeter::SetLengthScalingEnabled(bool e){ m_impl->lengthScaling=e; }
void AnimationRetargeter::SetRootMotionScale(float s){ m_impl->rootMotionScale=s; }
void AnimationRetargeter::SetUnmappedPolicy(bool keep){ m_impl->keepBindPose=keep; }
uint32_t AnimationRetargeter::MappingCount() const { return (uint32_t)m_impl->mapping.size(); }
std::string AnimationRetargeter::GetMappedTarget(const std::string& src) const {
    auto it=m_impl->mapping.find(src); return it!=m_impl->mapping.end()?it->second:"";
}

} // namespace Engine
