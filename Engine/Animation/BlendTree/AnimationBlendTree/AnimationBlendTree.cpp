#include "Engine/Animation/BlendTree/AnimationBlendTree/AnimationBlendTree.h"
#include "Engine/Animation/Skeleton/SkeletonSystem/SkeletonSystem.h"
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace Engine {

static float Lerp(float a, float b, float t) { return a+(b-a)*t; }

struct BlendTreeDef {
    uint32_t treeId{0};
    std::string skeletonDefId;
    std::vector<BlendNodeDesc> nodes;
    uint32_t rootNodeId{0};
    uint32_t nextNodeId{1};
};

struct BlendTreeInstance {
    uint32_t instanceId{0};
    uint32_t treeId{0};
    std::unordered_map<std::string,float> params;
    std::unordered_map<uint32_t, float>   nodeWeights;
    float clipTime{0.f};
};

static JointPose LerpPose(const JointPose& a, const JointPose& b, float t) {
    JointPose out;
    for(int i=0;i<3;i++) out.translation[i]=Lerp(a.translation[i],b.translation[i],t);
    for(int i=0;i<4;i++) out.rotation[i]=Lerp(a.rotation[i],b.rotation[i],t);
    for(int i=0;i<3;i++) out.scale[i]=Lerp(a.scale[i],b.scale[i],t);
    return out;
}

struct AnimationBlendTree::Impl {
    std::vector<BlendTreeDef>      trees;
    std::vector<BlendTreeInstance> instances;
    uint32_t nextTreeId{1};
    uint32_t nextInstId{1};
    ClipSampleFn clipSampleFn;

    BlendTreeDef* FindTree(uint32_t id) {
        for(auto& t:trees) if(t.treeId==id) return &t; return nullptr;
    }
    BlendTreeInstance* FindInst(uint32_t id) {
        for(auto& i:instances) if(i.instanceId==id) return &i; return nullptr;
    }

    void EvaluateNode(const BlendNodeDesc& node, BlendTreeInstance& inst,
                       float dt, std::vector<JointPose>& out)
    {
        switch (node.type) {
        case BlendNodeType::Clip: {
            inst.clipTime+=dt;
            if (clipSampleFn) clipSampleFn(node.clipId, inst.clipTime, out);
            break;
        }
        case BlendNodeType::BlendSpace1D: {
            if (node.entries1D.size()<2 || out.empty()) break;
            float param = 0.f;
            auto pit = inst.params.find(node.paramName);
            if (pit!=inst.params.end()) param=pit->second;
            // Find surrounding entries
            const auto& ents = node.entries1D;
            uint32_t lo=0, hi=0;
            for (uint32_t i=0;i+1<ents.size();i++) {
                if (param>=ents[i].position && param<=ents[i+1].position) { lo=i; hi=i+1; break; }
                if (i+1==ents.size()-1) { lo=i; hi=i+1; }
            }
            float range=ents[hi].position-ents[lo].position;
            float t=(range>0.f)?(param-ents[lo].position)/range:0.f;
            t=std::max(0.f,std::min(1.f,t));
            std::vector<JointPose> poseA(out.size()), poseB(out.size());
            if (clipSampleFn) {
                clipSampleFn(ents[lo].clipId, inst.clipTime, poseA);
                clipSampleFn(ents[hi].clipId, inst.clipTime, poseB);
            }
            for (uint32_t i=0;i<(uint32_t)out.size();i++) out[i]=LerpPose(poseA[i],poseB[i],t);
            break;
        }
        case BlendNodeType::Additive: {
            std::vector<JointPose> basePose(out.size()), delta(out.size());
            // Find base and additive nodes (by index in tree — simplified)
            auto* tree = FindTree(inst.treeId);
            if (tree) {
                for (auto& n : tree->nodes) {
                    if (n.type!=BlendNodeType::Additive) {
                        EvaluateNode(n, inst, dt, basePose);
                        break;
                    }
                }
            }
            float w = node.additiveWeight;
            auto wit = inst.nodeWeights.find(0);
            if (wit!=inst.nodeWeights.end()) w=wit->second;
            for (uint32_t i=0;i<(uint32_t)out.size();i++) out[i]=LerpPose(basePose[i],delta[i],w);
            break;
        }
        default:
            if (clipSampleFn && !node.clipId.empty()) clipSampleFn(node.clipId, inst.clipTime, out);
            break;
        }
    }
};

AnimationBlendTree::AnimationBlendTree()  : m_impl(new Impl) {}
AnimationBlendTree::~AnimationBlendTree() { Shutdown(); delete m_impl; }
void AnimationBlendTree::Init()     {}
void AnimationBlendTree::Shutdown() { m_impl->trees.clear(); m_impl->instances.clear(); }

uint32_t AnimationBlendTree::CreateTree(const std::string& skelDefId) {
    BlendTreeDef def; def.treeId=m_impl->nextTreeId++; def.skeletonDefId=skelDefId;
    m_impl->trees.push_back(def); return m_impl->trees.back().treeId;
}
void AnimationBlendTree::DestroyTree(uint32_t id) {
    auto& v=m_impl->trees; v.erase(std::remove_if(v.begin(),v.end(),[&](auto& t){return t.treeId==id;}),v.end());
}
uint32_t AnimationBlendTree::AddNode(uint32_t treeId, const BlendNodeDesc& desc) {
    auto* t=m_impl->FindTree(treeId); if(!t) return 0;
    BlendNodeDesc d=desc;
    uint32_t id=t->nextNodeId++;
    t->nodes.push_back(d);
    return id;
}
void AnimationBlendTree::SetRootNode(uint32_t treeId, uint32_t nodeId) {
    auto* t=m_impl->FindTree(treeId); if(t) t->rootNodeId=nodeId;
}
uint32_t AnimationBlendTree::AddClip(uint32_t tid, const std::string& name, const std::string& clipId) {
    BlendNodeDesc d; d.name=name; d.type=BlendNodeType::Clip; d.clipId=clipId;
    return AddNode(tid, d);
}
uint32_t AnimationBlendTree::Add1DBlendSpace(uint32_t tid, const std::string& param,
                                               const std::vector<std::string>& clips,
                                               const std::vector<float>& positions) {
    BlendNodeDesc d; d.type=BlendNodeType::BlendSpace1D; d.paramName=param;
    for (uint32_t i=0;i<(uint32_t)std::min(clips.size(),positions.size());i++)
        d.entries1D.push_back({clips[i], positions[i]});
    return AddNode(tid, d);
}
uint32_t AnimationBlendTree::Add2DBlendSpace(uint32_t tid, const std::string& px, const std::string& py,
                                               const std::vector<BlendSpaceEntry2D>& entries) {
    BlendNodeDesc d; d.type=BlendNodeType::BlendSpace2D; d.paramName=px; d.paramNameY=py; d.entries2D=entries;
    return AddNode(tid, d);
}
uint32_t AnimationBlendTree::AddAdditiveLayer(uint32_t tid, uint32_t baseId, uint32_t addId, float w) {
    BlendNodeDesc d; d.type=BlendNodeType::Additive; d.additiveWeight=w;
    d.baseNodeId=baseId; d.additiveNodeId=addId;
    return AddNode(tid, d);
}

uint32_t AnimationBlendTree::CreateInstance(uint32_t treeId) {
    auto* t=m_impl->FindTree(treeId); if(!t) return 0;
    BlendTreeInstance inst; inst.instanceId=m_impl->nextInstId++; inst.treeId=treeId;
    m_impl->instances.push_back(inst); return m_impl->instances.back().instanceId;
}
void AnimationBlendTree::DestroyInstance(uint32_t id) {
    auto& v=m_impl->instances; v.erase(std::remove_if(v.begin(),v.end(),[&](auto& i){return i.instanceId==id;}),v.end());
}
bool AnimationBlendTree::HasInstance(uint32_t id) const { return m_impl->FindInst(id)!=nullptr; }

void  AnimationBlendTree::SetParam(uint32_t instId, const std::string& name, float v) {
    auto* i=m_impl->FindInst(instId); if(i) i->params[name]=v;
}
float AnimationBlendTree::GetParam(uint32_t instId, const std::string& name) const {
    const auto* i=m_impl->FindInst(instId); if(!i) return 0.f;
    auto it=i->params.find(name); return it!=i->params.end()?it->second:0.f;
}
void  AnimationBlendTree::SetWeight(uint32_t instId, uint32_t nodeId, float w) {
    auto* i=m_impl->FindInst(instId); if(i) i->nodeWeights[nodeId]=w;
}

void AnimationBlendTree::SetClipSampleFn(ClipSampleFn fn) { m_impl->clipSampleFn=fn; }

void AnimationBlendTree::Evaluate(uint32_t instId, float dt, std::vector<JointPose>& outPose)
{
    auto* inst = m_impl->FindInst(instId); if(!inst) return;
    auto* tree = m_impl->FindTree(inst->treeId); if(!tree||tree->nodes.empty()) return;
    inst->clipTime+=dt;
    // Evaluate root node (index rootNodeId-1, or first node)
    uint32_t rootIdx = (tree->rootNodeId>0) ? tree->rootNodeId-1 : 0;
    if (rootIdx>=(uint32_t)tree->nodes.size()) rootIdx=0;
    m_impl->EvaluateNode(tree->nodes[rootIdx], *inst, dt, outPose);
}

bool AnimationBlendTree::LoadTree(const std::string& path) {
    std::ifstream f(path); return f.good();
}

} // namespace Engine
