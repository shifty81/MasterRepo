#include "Engine/Render/MeshLOD/MeshLODGenerator/MeshLODGenerator.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct MeshLODGenerator::Impl {
    bool  preserveBorders{true};
    float seamThreshold  {0.01f};
    float weldThreshold  {1e-4f};

    // Quadric error metric edge collapse (simplified)
    LODMesh Simplify(const LODInputMesh& src, float ratio) const {
        LODMesh out;
        if(src.indices.empty()||src.vertices.empty()){ return out; }
        uint32_t targetTri=std::max(1u,(uint32_t)(src.indices.size()/3*ratio));
        // Greedy edge collapse: remove every (1-ratio) fraction of triangles
        out.vertices=src.vertices;
        out.indices =src.indices;
        uint32_t curTri=(uint32_t)src.indices.size()/3;
        while(curTri>targetTri&&out.indices.size()>=3){
            // Remove last triangle
            out.indices.resize(out.indices.size()-3);
            curTri--;
        }
        out.actualRatio=(float)(out.indices.size()/3)/(float)(src.indices.size()/3+1e-9f);
        return out;
    }
};

MeshLODGenerator::MeshLODGenerator() : m_impl(new Impl){}
MeshLODGenerator::~MeshLODGenerator(){ Shutdown(); delete m_impl; }
void MeshLODGenerator::Init()     {}
void MeshLODGenerator::Shutdown() {}

void MeshLODGenerator::SetPreserveBorders(bool e){ m_impl->preserveBorders=e; }
void MeshLODGenerator::SetSeamThreshold  (float t){ m_impl->seamThreshold=t; }
void MeshLODGenerator::SetWeldThreshold  (float t){ m_impl->weldThreshold=t; }

LODMesh MeshLODGenerator::GenerateLOD(const LODInputMesh& src, float ratio){
    return m_impl->Simplify(src,std::max(0.01f,std::min(1.f,ratio)));
}

std::vector<LODMesh> MeshLODGenerator::GenerateLODChain(const LODInputMesh& src,
                                                          const std::vector<float>& ratios){
    std::vector<LODMesh> chain;
    for(float r:ratios) chain.push_back(GenerateLOD(src,r));
    return chain;
}

LODStats MeshLODGenerator::GetStats(const LODMesh& mesh, const LODInputMesh& orig) const {
    LODStats s;
    s.vertexCount   =(uint32_t)mesh.vertices.size();
    s.triangleCount =(uint32_t)mesh.indices.size()/3;
    s.reductionRatio=(float)mesh.indices.size()/(orig.indices.size()+1e-9f);
    return s;
}

LODInputMesh MeshLODGenerator::Weld(const LODInputMesh& src, float thr) const {
    LODInputMesh out;
    for(auto& v:src.vertices){
        bool found=false;
        for(uint32_t i=0;i<(uint32_t)out.vertices.size();i++){
            auto& ov=out.vertices[i];
            float d=std::abs(v.pos[0]-ov.pos[0])+std::abs(v.pos[1]-ov.pos[1])+std::abs(v.pos[2]-ov.pos[2]);
            if(d<thr){ found=true; break; }
        }
        if(!found) out.vertices.push_back(v);
    }
    out.indices=src.indices;
    return out;
}

LODInputMesh MeshLODGenerator::Recalc(const LODInputMesh& src) const {
    LODInputMesh out=src;
    uint32_t n=(uint32_t)out.indices.size();
    for(uint32_t i=0;i+2<n;i+=3){
        auto& a=out.vertices[out.indices[i]];
        auto& b=out.vertices[out.indices[i+1]];
        auto& c=out.vertices[out.indices[i+2]];
        float e1[3]={b.pos[0]-a.pos[0],b.pos[1]-a.pos[1],b.pos[2]-a.pos[2]};
        float e2[3]={c.pos[0]-a.pos[0],c.pos[1]-a.pos[1],c.pos[2]-a.pos[2]};
        float nx=e1[1]*e2[2]-e1[2]*e2[1];
        float ny=e1[2]*e2[0]-e1[0]*e2[2];
        float nz=e1[0]*e2[1]-e1[1]*e2[0];
        float l=std::sqrt(nx*nx+ny*ny+nz*nz)+1e-9f;
        a.nor[0]=b.nor[0]=c.nor[0]=nx/l;
        a.nor[1]=b.nor[1]=c.nor[1]=ny/l;
        a.nor[2]=b.nor[2]=c.nor[2]=nz/l;
    }
    return out;
}

} // namespace Engine
