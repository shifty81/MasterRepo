#include "Engine/Render/MeshBuilder/MeshBuilder/MeshBuilder.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Engine {

void MeshBuilder::Clear() { m_verts.clear(); m_indices.clear(); }

uint32_t MeshBuilder::AddVertex(const MBVertex& v)
{
    m_verts.push_back(v); return (uint32_t)m_verts.size()-1;
}

uint32_t MeshBuilder::AddVertex(const float pos[3], const float uv[2],
                                  const float normal[3])
{
    MBVertex v;
    if (pos)    { v.position[0]=pos[0]; v.position[1]=pos[1]; v.position[2]=pos[2]; }
    if (uv)     { v.uv0[0]=uv[0];  v.uv0[1]=uv[1]; }
    if (normal) { v.normal[0]=normal[0]; v.normal[1]=normal[1]; v.normal[2]=normal[2]; }
    return AddVertex(v);
}

void MeshBuilder::AddTriangle(uint32_t a, uint32_t b, uint32_t c)
{
    m_indices.push_back(a); m_indices.push_back(b); m_indices.push_back(c);
}

void MeshBuilder::AddQuad(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    AddTriangle(a,b,c); AddTriangle(a,c,d);
}

void MeshBuilder::AddBox(const float centre[3], const float half[3])
{
    const float cx=centre[0], cy=centre[1], cz=centre[2];
    const float hx=half[0],  hy=half[1],  hz=half[2];

    struct Face { float n[3]; uint32_t vi[4]; };
    // Define 8 corners: 0=---,1=+--,2=++-,3=-+-,4=--+,5=+-+,6=+++,7=-++
    float corners[8][3] = {
        {cx-hx,cy-hy,cz-hz},{cx+hx,cy-hy,cz-hz},{cx+hx,cy+hy,cz-hz},{cx-hx,cy+hy,cz-hz},
        {cx-hx,cy-hy,cz+hz},{cx+hx,cy-hy,cz+hz},{cx+hx,cy+hy,cz+hz},{cx-hx,cy+hy,cz+hz}
    };
    float uvs[4][2]={{0,0},{1,0},{1,1},{0,1}};
    int faces[6][4]={{3,2,1,0},{4,5,6,7},{0,1,5,4},{2,3,7,6},{0,4,7,3},{1,2,6,5}};
    float normals[6][3]={{0,0,-1},{0,0,1},{0,-1,0},{0,1,0},{-1,0,0},{1,0,0}};
    for (int f=0;f<6;f++) {
        uint32_t base=(uint32_t)m_verts.size();
        for (int i=0;i<4;i++) {
            MBVertex v;
            v.position[0]=corners[faces[f][i]][0];
            v.position[1]=corners[faces[f][i]][1];
            v.position[2]=corners[faces[f][i]][2];
            v.normal[0]=normals[f][0]; v.normal[1]=normals[f][1]; v.normal[2]=normals[f][2];
            v.uv0[0]=uvs[i][0]; v.uv0[1]=uvs[i][1];
            AddVertex(v);
        }
        AddQuad(base, base+1, base+2, base+3);
    }
}

void MeshBuilder::AddSphere(const float centre[3], float radius,
                              uint32_t stacks, uint32_t slices)
{
    for (uint32_t s=0; s<=stacks; s++) {
        float phi = (float)(M_PI * s / stacks);
        for (uint32_t sl=0; sl<=slices; sl++) {
            float theta = (float)(2*M_PI * sl / slices);
            MBVertex v;
            v.normal[0] = std::sin(phi)*std::cos(theta);
            v.normal[1] = std::cos(phi);
            v.normal[2] = std::sin(phi)*std::sin(theta);
            v.position[0]=centre[0]+radius*v.normal[0];
            v.position[1]=centre[1]+radius*v.normal[1];
            v.position[2]=centre[2]+radius*v.normal[2];
            v.uv0[0]=(float)sl/slices; v.uv0[1]=(float)s/stacks;
            AddVertex(v);
        }
    }
    for (uint32_t s=0;s<stacks;s++) for (uint32_t sl=0;sl<slices;sl++) {
        uint32_t a=(s  )*(slices+1)+sl, b=a+1;
        uint32_t c=(s+1)*(slices+1)+sl, d=c+1;
        AddTriangle(a,b,d); AddTriangle(a,d,c);
    }
}

void MeshBuilder::AddPlane(const float centre[3], const float /*normal*/[3],
                             float halfW, float halfH, uint32_t divsW, uint32_t divsH)
{
    for (uint32_t y=0;y<=divsH;y++) for (uint32_t x=0;x<=divsW;x++) {
        MBVertex v;
        v.position[0]=centre[0]-halfW + 2*halfW*(float)x/divsW;
        v.position[1]=centre[1];
        v.position[2]=centre[2]-halfH + 2*halfH*(float)y/divsH;
        v.normal[1]=1.f;
        v.uv0[0]=(float)x/divsW; v.uv0[1]=(float)y/divsH;
        AddVertex(v);
    }
    for (uint32_t y=0;y<divsH;y++) for (uint32_t x=0;x<divsW;x++) {
        uint32_t a=y*(divsW+1)+x, b=a+1, c=(y+1)*(divsW+1)+x, d=c+1;
        AddQuad(a,b,d,c);
    }
}

void MeshBuilder::AddCylinder(const float base[3], float radius, float height,
                                uint32_t slices, bool caps)
{
    uint32_t b0=(uint32_t)m_verts.size();
    for (uint32_t i=0;i<=slices;i++) {
        float theta=(float)(2*M_PI*i/slices);
        float cx=radius*std::cos(theta), cz=radius*std::sin(theta);
        for (int layer=0;layer<2;layer++) {
            MBVertex v;
            v.position[0]=base[0]+cx; v.position[1]=base[1]+height*layer; v.position[2]=base[2]+cz;
            v.normal[0]=cx/radius; v.normal[2]=cz/radius;
            v.uv0[0]=(float)i/slices; v.uv0[1]=(float)layer;
            AddVertex(v);
        }
    }
    for (uint32_t i=0;i<slices;i++) {
        uint32_t a=b0+i*2, b=b0+i*2+1, c=b0+(i+1)*2, d=b0+(i+1)*2+1;
        AddQuad(a,c,d,b);
    }
    (void)caps;
}

void MeshBuilder::AddCapsule(const float base[3], float radius, float height,
                               uint32_t stacks, uint32_t slices)
{
    // Top hemisphere
    float topCentre[3]={base[0], base[1]+height, base[2]};
    AddSphere(topCentre, radius, stacks/2, slices);
    // Cylinder body
    AddCylinder(base, radius, height, slices, false);
}

void MeshBuilder::AddCone(const float base[3], float radius, float height,
                           uint32_t slices, bool cap)
{
    uint32_t apex = AddVertex({base[0], base[1]+height, base[2]});
    uint32_t b0   = (uint32_t)m_verts.size();
    for (uint32_t i=0;i<=slices;i++) {
        float theta=(float)(2*M_PI*i/slices);
        float pos[3]={base[0]+radius*std::cos(theta), base[1], base[2]+radius*std::sin(theta)};
        AddVertex(pos, nullptr, nullptr);
    }
    for (uint32_t i=0;i<slices;i++) AddTriangle(apex, b0+i+1, b0+i);
    (void)cap;
}

void MeshBuilder::RecalcNormals()
{
    for (auto& v : m_verts) { v.normal[0]=0; v.normal[1]=0; v.normal[2]=0; }
    for (uint32_t i=0;i+2<(uint32_t)m_indices.size();i+=3) {
        auto& va=m_verts[m_indices[i]], &vb=m_verts[m_indices[i+1]], &vc=m_verts[m_indices[i+2]];
        float ab[3]={vb.position[0]-va.position[0],vb.position[1]-va.position[1],vb.position[2]-va.position[2]};
        float ac[3]={vc.position[0]-va.position[0],vc.position[1]-va.position[1],vc.position[2]-va.position[2]};
        float n[3]={ab[1]*ac[2]-ab[2]*ac[1], ab[2]*ac[0]-ab[0]*ac[2], ab[0]*ac[1]-ab[1]*ac[0]};
        for (int k=0;k<3;k++) { va.normal[k]+=n[k]; vb.normal[k]+=n[k]; vc.normal[k]+=n[k]; }
    }
    for (auto& v : m_verts) {
        float len=std::sqrt(v.normal[0]*v.normal[0]+v.normal[1]*v.normal[1]+v.normal[2]*v.normal[2]);
        if (len>1e-6f) { v.normal[0]/=len; v.normal[1]/=len; v.normal[2]/=len; }
    }
}

void MeshBuilder::RecalcTangents()
{
    for (auto& v : m_verts) { v.tangent[0]=1; v.tangent[1]=0; v.tangent[2]=0; v.tangent[3]=1; }
}

void MeshBuilder::FlipNormals()
{
    for (auto& v : m_verts) { v.normal[0]=-v.normal[0]; v.normal[1]=-v.normal[1]; v.normal[2]=-v.normal[2]; }
}

void MeshBuilder::FlipWinding()
{
    for (uint32_t i=0;i+2<(uint32_t)m_indices.size();i+=3) std::swap(m_indices[i+1], m_indices[i+2]);
}

void MeshBuilder::Translate(const float d[3])
{
    for (auto& v : m_verts) { v.position[0]+=d[0]; v.position[1]+=d[1]; v.position[2]+=d[2]; }
}

void MeshBuilder::Scale(const float s[3])
{
    for (auto& v : m_verts) { v.position[0]*=s[0]; v.position[1]*=s[1]; v.position[2]*=s[2]; }
}

void MeshBuilder::Rotate(const float /*axisAngle*/[4]) {}

void MeshBuilder::Merge(const MeshBuilder& other)
{
    uint32_t offset = (uint32_t)m_verts.size();
    for (auto& v : other.m_verts) m_verts.push_back(v);
    for (auto idx : other.m_indices) m_indices.push_back(idx+offset);
}

std::vector<float> MeshBuilder::GetPositions() const
{
    std::vector<float> out; out.reserve(m_verts.size()*3);
    for (auto& v : m_verts) { out.push_back(v.position[0]); out.push_back(v.position[1]); out.push_back(v.position[2]); }
    return out;
}
std::vector<float> MeshBuilder::GetNormals() const
{
    std::vector<float> out; out.reserve(m_verts.size()*3);
    for (auto& v : m_verts) { out.push_back(v.normal[0]); out.push_back(v.normal[1]); out.push_back(v.normal[2]); }
    return out;
}
std::vector<float> MeshBuilder::GetTangents() const
{
    std::vector<float> out; out.reserve(m_verts.size()*4);
    for (auto& v : m_verts) { out.push_back(v.tangent[0]); out.push_back(v.tangent[1]); out.push_back(v.tangent[2]); out.push_back(v.tangent[3]); }
    return out;
}
std::vector<float> MeshBuilder::GetUV0() const
{
    std::vector<float> out; out.reserve(m_verts.size()*2);
    for (auto& v : m_verts) { out.push_back(v.uv0[0]); out.push_back(v.uv0[1]); }
    return out;
}
std::vector<float> MeshBuilder::GetColours() const
{
    std::vector<float> out; out.reserve(m_verts.size()*4);
    for (auto& v : m_verts) { for(int i=0;i<4;i++) out.push_back(v.colour[i]); }
    return out;
}
std::vector<uint32_t> MeshBuilder::GetIndices() const  { return m_indices; }
std::vector<MBVertex> MeshBuilder::GetVertices() const { return m_verts; }
uint32_t MeshBuilder::VertexCount()   const { return (uint32_t)m_verts.size(); }
uint32_t MeshBuilder::TriangleCount() const { return (uint32_t)m_indices.size()/3; }
bool     MeshBuilder::IsEmpty()       const { return m_verts.empty(); }

MBBounds MeshBuilder::GetBounds() const
{
    MBBounds b;
    if (m_verts.empty()) return b;
    for(int i=0;i<3;i++){b.min[i]=1e9f; b.max[i]=-1e9f;}
    for (auto& v : m_verts) for(int i=0;i<3;i++) {
        b.min[i]=std::min(b.min[i],v.position[i]);
        b.max[i]=std::max(b.max[i],v.position[i]);
    }
    return b;
}

} // namespace Engine
