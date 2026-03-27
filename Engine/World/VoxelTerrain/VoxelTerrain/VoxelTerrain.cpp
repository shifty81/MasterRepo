#include "Engine/World/VoxelTerrain/VoxelTerrain/VoxelTerrain.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace Engine {

struct ChunkData {
    uint32_t idx{0};
    std::vector<uint8_t> voxels; // chunkSize^3
    VoxelMesh mesh;
    bool dirty{true};
};

struct TerrainData {
    uint32_t         id{0};
    VoxelTerrainDesc desc;
    float            origin[3]{};
    // Voxels stored flat [z*sizeX*sizeY + y*sizeX + x]
    std::vector<uint8_t> voxels;
    std::vector<ChunkData> chunks;
    bool hasMesh{false};
    std::function<void(uint32_t,uint32_t)> onChunkUpdated;

    int32_t  CW()  const { return (int32_t)desc.chunkSize; }
    uint32_t CX()  const { return (desc.sizeX+desc.chunkSize-1)/desc.chunkSize; }
    uint32_t CY()  const { return (desc.sizeY+desc.chunkSize-1)/desc.chunkSize; }
    uint32_t CZ()  const { return (desc.sizeZ+desc.chunkSize-1)/desc.chunkSize; }
    uint32_t ChunkIndex(uint32_t cx,uint32_t cy,uint32_t cz) const {
        return cz*CX()*CY()+cy*CX()+cx;
    }
    bool InBounds(int32_t x,int32_t y,int32_t z) const {
        return x>=0&&y>=0&&z>=0&&(uint32_t)x<desc.sizeX&&(uint32_t)y<desc.sizeY&&(uint32_t)z<desc.sizeZ;
    }
    uint8_t Get(int32_t x,int32_t y,int32_t z) const {
        if(!InBounds(x,y,z)) return 0;
        return voxels[(uint32_t)z*desc.sizeX*desc.sizeY+(uint32_t)y*desc.sizeX+(uint32_t)x];
    }
    void Set(int32_t x,int32_t y,int32_t z,uint8_t m){
        if(!InBounds(x,y,z)) return;
        voxels[(uint32_t)z*desc.sizeX*desc.sizeY+(uint32_t)y*desc.sizeX+(uint32_t)x]=m;
        // Mark chunk dirty
        uint32_t cx=(uint32_t)x/desc.chunkSize;
        uint32_t cy=(uint32_t)y/desc.chunkSize;
        uint32_t cz=(uint32_t)z/desc.chunkSize;
        auto ci=ChunkIndex(cx,cy,cz);
        if(ci<chunks.size()) chunks[ci].dirty=true;
    }
};

struct VoxelTerrain::Impl {
    std::vector<TerrainData*> terrains;
    uint32_t nextId{1};

    TerrainData* Find(uint32_t id){
        for(auto* t:terrains) if(t->id==id) return t; return nullptr;
    }
    const TerrainData* Find(uint32_t id) const {
        for(auto* t:terrains) if(t->id==id) return t; return nullptr;
    }

    // Simplified marching-cubes: just emits quads for exposed faces
    void GenChunkMesh(TerrainData& t, ChunkData& ch, uint32_t ci){
        ch.mesh.vertices.clear(); ch.mesh.indices.clear(); ch.mesh.chunkIdx=ci;
        uint32_t cx=(ci % t.CX()) * t.desc.chunkSize;
        uint32_t cz=(ci / t.CX() / t.CY()) * t.desc.chunkSize;
        uint32_t cy=(ci / t.CX() % t.CY()) * t.desc.chunkSize;
        float vs=t.desc.voxelSize;
        uint32_t vtx=0;
        static const int32_t faces[6][3]={ {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1} };
        static const float   normals[6][3]={ {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1} };
        for(uint32_t lz=0;lz<t.desc.chunkSize;lz++) for(uint32_t ly=0;ly<t.desc.chunkSize;ly++)
        for(uint32_t lx=0;lx<t.desc.chunkSize;lx++){
            int32_t gx=cx+lx, gy=cy+ly, gz=cz+lz;
            if(!t.Get(gx,gy,gz)) continue;
            for(int f=0;f<6;f++){
                if(t.Get(gx+faces[f][0],gy+faces[f][1],gz+faces[f][2])) continue;
                // Push quad (2 triangles)
                float px=(float)gx*vs+t.origin[0];
                float py=(float)gy*vs+t.origin[1];
                float pz=(float)gz*vs+t.origin[2];
                for(int v=0;v<4;v++){
                    float ox=(v==1||v==2)?vs:0;
                    float oy=(v==2||v==3)?vs:0;
                    float oz=0;
                    ch.mesh.vertices.insert(ch.mesh.vertices.end(),{px+ox,py+oy,pz+oz,
                        normals[f][0],normals[f][1],normals[f][2]});
                }
                ch.mesh.indices.insert(ch.mesh.indices.end(),{vtx,vtx+1,vtx+2,vtx,vtx+2,vtx+3});
                vtx+=4;
            }
        }
        ch.dirty=false;
    }
};

VoxelTerrain::VoxelTerrain()  : m_impl(new Impl){}
VoxelTerrain::~VoxelTerrain() { Shutdown(); delete m_impl; }
void VoxelTerrain::Init()     {}
void VoxelTerrain::Shutdown() { for(auto* t:m_impl->terrains) delete t; m_impl->terrains.clear(); }

uint32_t VoxelTerrain::Create(const VoxelTerrainDesc& desc, const float origin[3])
{
    auto* t=new TerrainData; t->id=m_impl->nextId++; t->desc=desc;
    if(origin) for(int i=0;i<3;i++) t->origin[i]=origin[i];
    t->voxels.assign((size_t)desc.sizeX*desc.sizeY*desc.sizeZ, 0);
    uint32_t nc=t->CX()*t->CY()*t->CZ();
    t->chunks.resize(nc);
    for(uint32_t i=0;i<nc;i++){t->chunks[i].idx=i; t->chunks[i].dirty=true;}
    m_impl->terrains.push_back(t); return t->id;
}

void VoxelTerrain::Destroy(uint32_t id){
    auto& v=m_impl->terrains;
    for(auto it=v.begin();it!=v.end();++it) if((*it)->id==id){delete *it;v.erase(it);return;}
}
bool VoxelTerrain::Has(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void    VoxelTerrain::SetVoxel(uint32_t id,int32_t x,int32_t y,int32_t z,uint8_t m){
    auto* t=m_impl->Find(id); if(t) t->Set(x,y,z,m);
}
uint8_t VoxelTerrain::GetVoxel(uint32_t id,int32_t x,int32_t y,int32_t z) const {
    const auto* t=m_impl->Find(id); return t?t->Get(x,y,z):0;
}

void VoxelTerrain::FillBox(uint32_t id, const int32_t mn[3], const int32_t mx[3], uint8_t mat){
    auto* t=m_impl->Find(id); if(!t) return;
    for(int32_t z=mn[2];z<=mx[2];z++) for(int32_t y=mn[1];y<=mx[1];y++)
        for(int32_t x=mn[0];x<=mx[0];x++) t->Set(x,y,z,mat);
}

void VoxelTerrain::FillSphere(uint32_t id, const float c[3], float r, uint8_t mat){
    auto* t=m_impl->Find(id); if(!t) return;
    float vs=t->desc.voxelSize;
    int32_t ri=(int32_t)(r/vs)+1;
    int32_t cx_=(int32_t)(c[0]/vs), cy_=(int32_t)(c[1]/vs), cz_=(int32_t)(c[2]/vs);
    for(int32_t z=cz_-ri;z<=cz_+ri;z++) for(int32_t y=cy_-ri;y<=cy_+ri;y++)
    for(int32_t x=cx_-ri;x<=cx_+ri;x++){
        float dx=(x-cx_)*vs, dy=(y-cy_)*vs, dz=(z-cz_)*vs;
        if(dx*dx+dy*dy+dz*dz<=r*r) t->Set(x,y,z,mat);
    }
}

void VoxelTerrain::RegenerateDirty(uint32_t id){
    auto* t=m_impl->Find(id); if(!t) return;
    for(uint32_t i=0;i<(uint32_t)t->chunks.size();i++)
        if(t->chunks[i].dirty){
            m_impl->GenChunkMesh(*t,t->chunks[i],i);
            if(t->onChunkUpdated) t->onChunkUpdated(id,i);
        }
}
void VoxelTerrain::RegenerateAll(uint32_t id){
    auto* t=m_impl->Find(id); if(!t) return;
    for(auto& ch:t->chunks){ ch.dirty=true; }
    RegenerateDirty(id);
}

const VoxelMesh* VoxelTerrain::GetMesh(uint32_t id, uint32_t ci) const {
    const auto* t=m_impl->Find(id); if(!t||ci>=t->chunks.size()) return nullptr;
    return &t->chunks[ci].mesh;
}
uint32_t VoxelTerrain::ChunkCount(uint32_t id) const {
    const auto* t=m_impl->Find(id); return t?(uint32_t)t->chunks.size():0;
}
bool VoxelTerrain::IsChunkDirty(uint32_t id, uint32_t ci) const {
    const auto* t=m_impl->Find(id); return t&&ci<t->chunks.size()&&t->chunks[ci].dirty;
}

bool VoxelTerrain::Raycast(uint32_t id, const float org[3], const float dir[3],
                             float maxDist, float outPos[3], uint8_t& outMat) const
{
    const auto* t=m_impl->Find(id); if(!t) return false;
    float vs=t->desc.voxelSize;
    float step=vs*0.5f;
    float d=0.f;
    while(d<maxDist){
        float px=org[0]+dir[0]*d, py=org[1]+dir[1]*d, pz=org[2]+dir[2]*d;
        int32_t x=(int32_t)((px-t->origin[0])/vs);
        int32_t y=(int32_t)((py-t->origin[1])/vs);
        int32_t z=(int32_t)((pz-t->origin[2])/vs);
        uint8_t m=t->Get(x,y,z);
        if(m){
            if(outPos){outPos[0]=px;outPos[1]=py;outPos[2]=pz;}
            outMat=m; return true;
        }
        d+=step;
    }
    return false;
}

bool VoxelTerrain::Save(uint32_t id, const std::string& path) const {
    const auto* t=m_impl->Find(id); if(!t) return false;
    std::ofstream f(path,std::ios::binary); if(!f) return false;
    f.write((char*)t->voxels.data(),(std::streamsize)t->voxels.size());
    return true;
}
bool VoxelTerrain::Load(uint32_t id, const std::string& path){
    auto* t=m_impl->Find(id); if(!t) return false;
    std::ifstream f(path,std::ios::binary); if(!f) return false;
    f.read((char*)t->voxels.data(),(std::streamsize)t->voxels.size());
    for(auto& ch:t->chunks) ch.dirty=true;
    return true;
}

void VoxelTerrain::SetOnChunkUpdated(std::function<void(uint32_t,uint32_t)> cb){
    // Apply to all terrains created so far (and store for future)
    for(auto* t:m_impl->terrains) t->onChunkUpdated=cb;
}
std::vector<uint32_t> VoxelTerrain::GetAll() const {
    std::vector<uint32_t> out; for(auto* t:m_impl->terrains) out.push_back(t->id); return out;
}

} // namespace Engine
