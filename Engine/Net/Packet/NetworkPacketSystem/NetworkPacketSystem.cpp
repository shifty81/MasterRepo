#include "Engine/Net/Packet/NetworkPacketSystem/NetworkPacketSystem.h"
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Engine {

struct PacketData {
    uint32_t              id;
    uint32_t              typeId{0};
    uint32_t              seqNum{0};
    std::vector<uint8_t>  payload;
    uint32_t              readCursor{0};
};

struct NetworkPacketSystem::Impl {
    std::unordered_map<uint32_t,std::string>  typeNames;
    std::unordered_map<uint32_t,PacketData>   packets;
    std::unordered_set<uint32_t>              acked;
    std::unordered_map<uint32_t,std::function<void(uint32_t)>> dispatch;
    uint32_t nextPid{1};
    uint32_t nextSeq{1};

    PacketData* Find(uint32_t id){ auto it=packets.find(id); return it!=packets.end()?&it->second:nullptr; }
};

NetworkPacketSystem::NetworkPacketSystem(): m_impl(new Impl){}
NetworkPacketSystem::~NetworkPacketSystem(){ Shutdown(); delete m_impl; }
void NetworkPacketSystem::Init(){}
void NetworkPacketSystem::Shutdown(){ m_impl->packets.clear(); m_impl->typeNames.clear(); }
void NetworkPacketSystem::Reset(){ m_impl->packets.clear(); m_impl->acked.clear(); m_impl->nextSeq=1; m_impl->nextPid=1; }

bool NetworkPacketSystem::RegisterPacketType(uint32_t typeId, const std::string& name){
    if(m_impl->typeNames.count(typeId)) return false;
    m_impl->typeNames[typeId]=name; return true;
}

uint32_t NetworkPacketSystem::CreatePacket(uint32_t typeId, uint32_t seqNum){
    uint32_t pid=m_impl->nextPid++;
    PacketData p; p.id=pid; p.typeId=typeId; p.seqNum=seqNum;
    m_impl->packets[pid]=p;
    return pid;
}
void NetworkPacketSystem::DropPacket(uint32_t pid){ m_impl->packets.erase(pid); }

// Write helpers
void NetworkPacketSystem::WriteUInt8(uint32_t pid, uint8_t v){
    auto* p=m_impl->Find(pid); if(!p) return;
    p->payload.push_back(v);
}
void NetworkPacketSystem::WriteUInt16(uint32_t pid, uint16_t v){
    auto* p=m_impl->Find(pid); if(!p) return;
    p->payload.push_back((uint8_t)(v>>8));
    p->payload.push_back((uint8_t)(v));
}
void NetworkPacketSystem::WriteUInt32(uint32_t pid, uint32_t v){
    auto* p=m_impl->Find(pid); if(!p) return;
    for(int i=3;i>=0;i--) p->payload.push_back((uint8_t)(v>>(i*8)));
}
void NetworkPacketSystem::WriteFloat(uint32_t pid, float v){
    uint32_t bits; std::memcpy(&bits,&v,4);
    WriteUInt32(pid,bits);
}
void NetworkPacketSystem::WriteString(uint32_t pid, const std::string& s){
    WriteUInt16(pid,(uint16_t)s.size());
    auto* p=m_impl->Find(pid); if(!p) return;
    for(char c:s) p->payload.push_back((uint8_t)c);
}

// Read helpers
bool NetworkPacketSystem::ReadUInt8(uint32_t pid, uint8_t& out){
    auto* p=m_impl->Find(pid); if(!p||p->readCursor>=p->payload.size()) return false;
    out=p->payload[p->readCursor++]; return true;
}
bool NetworkPacketSystem::ReadUInt16(uint32_t pid, uint16_t& out){
    auto* p=m_impl->Find(pid); if(!p||p->readCursor+1>=p->payload.size()) return false;
    out=((uint16_t)p->payload[p->readCursor]<<8)|p->payload[p->readCursor+1];
    p->readCursor+=2; return true;
}
bool NetworkPacketSystem::ReadUInt32(uint32_t pid, uint32_t& out){
    auto* p=m_impl->Find(pid); if(!p||p->readCursor+3>=p->payload.size()) return false;
    out=0; for(int i=0;i<4;i++) out=(out<<8)|p->payload[p->readCursor++];
    return true;
}
bool NetworkPacketSystem::ReadFloat(uint32_t pid, float& out){
    uint32_t bits; if(!ReadUInt32(pid,bits)) return false;
    std::memcpy(&out,&bits,4); return true;
}
bool NetworkPacketSystem::ReadString(uint32_t pid, std::string& out){
    uint16_t len; if(!ReadUInt16(pid,len)) return false;
    auto* p=m_impl->Find(pid); if(!p||p->readCursor+len>p->payload.size()) return false;
    out.assign(p->payload.begin()+p->readCursor,p->payload.begin()+p->readCursor+len);
    p->readCursor+=len; return true;
}

// Serialization: [typeId(4)][seqNum(4)][payload...]
uint32_t NetworkPacketSystem::Serialize(uint32_t pid, std::vector<uint8_t>& out) const {
    auto it=m_impl->packets.find(pid); if(it==m_impl->packets.end()) return 0;
    auto& p=it->second;
    out.clear();
    auto push32=[&](uint32_t v){ for(int i=3;i>=0;i--) out.push_back((uint8_t)(v>>(i*8))); };
    push32(p.typeId); push32(p.seqNum);
    out.insert(out.end(),p.payload.begin(),p.payload.end());
    return (uint32_t)out.size();
}
uint32_t NetworkPacketSystem::Deserialize(const std::vector<uint8_t>& bytes){
    if(bytes.size()<8) return 0;
    uint32_t tid=0,seq=0;
    for(int i=0;i<4;i++) tid=(tid<<8)|bytes[i];
    for(int i=0;i<4;i++) seq=(seq<<8)|bytes[4+i];
    uint32_t pid=CreatePacket(tid,seq);
    auto* p=m_impl->Find(pid);
    p->payload.assign(bytes.begin()+8,bytes.end());
    return pid;
}

uint32_t NetworkPacketSystem::Fragment(uint32_t pid, uint32_t mtu,
                                        std::vector<uint32_t>& frags){
    frags.clear(); auto* p=m_impl->Find(pid); if(!p) return 0;
    if(p->payload.size()<=mtu){ frags.push_back(pid); return 1; }
    uint32_t offset=0;
    while(offset<p->payload.size()){
        uint32_t sz=std::min(mtu,(uint32_t)(p->payload.size()-offset));
        uint32_t fid=CreatePacket(p->typeId,p->seqNum);
        auto* fp=m_impl->Find(fid);
        fp->payload.assign(p->payload.begin()+offset,p->payload.begin()+offset+sz);
        frags.push_back(fid); offset+=sz;
    }
    return (uint32_t)frags.size();
}
uint32_t NetworkPacketSystem::Reassemble(const std::vector<uint32_t>& fids){
    if(fids.empty()) return 0;
    auto* first=m_impl->Find(fids[0]); if(!first) return 0;
    uint32_t pid=CreatePacket(first->typeId,first->seqNum);
    auto* out=m_impl->Find(pid);
    for(auto fid:fids){
        auto* fp=m_impl->Find(fid); if(!fp) continue;
        out->payload.insert(out->payload.end(),fp->payload.begin(),fp->payload.end());
    }
    return pid;
}

void NetworkPacketSystem::SendAck(uint32_t seq){ m_impl->acked.insert(seq); }
bool NetworkPacketSystem::IsAcked(uint32_t seq) const { return m_impl->acked.count(seq)>0; }
uint32_t NetworkPacketSystem::GetNextSeqNum(){ return m_impl->nextSeq++; }

uint32_t NetworkPacketSystem::GetPacketTypeId(uint32_t pid) const {
    auto* p=m_impl->Find(pid); return p?p->typeId:0;
}
uint32_t NetworkPacketSystem::GetPacketSize(uint32_t pid) const {
    auto* p=m_impl->Find(pid); return p?(uint32_t)p->payload.size():0;
}

void NetworkPacketSystem::SetOnReceive(uint32_t typeId,
                                        std::function<void(uint32_t)> cb){
    m_impl->dispatch[typeId]=cb;
}
void NetworkPacketSystem::DispatchReceived(uint32_t pid){
    auto* p=m_impl->Find(pid); if(!p) return;
    auto it=m_impl->dispatch.find(p->typeId);
    if(it!=m_impl->dispatch.end()) it->second(pid);
}

} // namespace Engine
