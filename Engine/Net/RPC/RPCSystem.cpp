#include "Engine/Net/RPC/RPCSystem.h"
#include <algorithm>
#include <cstring>
#include <mutex>
#include <queue>
#include <unordered_map>

namespace Engine {

// ── RPCArgs ───────────────────────────────────────────────────────────────────

static void WriteRaw(std::vector<uint8_t>& data, const void* src, size_t n){
    const uint8_t* p=static_cast<const uint8_t*>(src);
    data.insert(data.end(),p,p+n);
}
template<typename T> static T ReadRaw(const std::vector<uint8_t>& data, size_t& pos){
    T v; if(pos+sizeof(T)>data.size()) return T{};
    std::memcpy(&v,data.data()+pos,sizeof(T)); pos+=sizeof(T); return v;
}

void RPCArgs::WriteBool(bool v)             { uint8_t b=v?1:0; WriteRaw(data,&b,1); }
void RPCArgs::WriteInt32(int32_t v)         { WriteRaw(data,&v,4); }
void RPCArgs::WriteFloat(float v)           { WriteRaw(data,&v,4); }
void RPCArgs::WriteVec3(const float v[3])   { WriteRaw(data,v,12); }
void RPCArgs::WriteBytes(const uint8_t* b, uint32_t n){ WriteRaw(data,&n,4); WriteRaw(data,b,n); }
void RPCArgs::WriteString(const std::string& s){
    uint32_t len=(uint32_t)s.size(); WriteRaw(data,&len,4);
    WriteRaw(data,s.data(),s.size());
}
bool        RPCArgs::ReadBool()   { return ReadRaw<uint8_t>(data,readPos)!=0; }
int32_t     RPCArgs::ReadInt32()  { return ReadRaw<int32_t>(data,readPos); }
float       RPCArgs::ReadFloat()  { return ReadRaw<float>(data,readPos); }
void RPCArgs::ReadVec3(float out[3]){
    for(int i=0;i<3;++i) out[i]=ReadRaw<float>(data,readPos);
}
std::string RPCArgs::ReadString(){
    uint32_t len=ReadRaw<uint32_t>(data,readPos);
    if(readPos+len>data.size()) return {};
    std::string s((char*)data.data()+readPos,len); readPos+=len; return s;
}
std::vector<uint8_t> RPCArgs::ReadBytes(){
    uint32_t len=ReadRaw<uint32_t>(data,readPos);
    if(readPos+len>data.size()) return {};
    std::vector<uint8_t> out(data.begin()+readPos,data.begin()+readPos+len);
    readPos+=len; return out;
}

// ── Pimpl ─────────────────────────────────────────────────────────────────────

struct PendingRPC {
    uint32_t    peerId{0};
    std::string name;
    std::vector<uint8_t> payload;
};

struct RPCSystem::Impl {
    std::unordered_map<std::string,RPCHandler> handlers;
    std::queue<PendingRPC>  incoming;
    std::mutex              mutex;
    std::function<void(const std::string&,const std::string&)> onError;

    // Simulate outgoing (in real impl this would send over network)
    void SendOutgoing(uint32_t peerId, const std::string& name, RPCArgs& args, RPCMode mode){
        (void)mode; (void)peerId;
        // Loopback for testing — inject as incoming from peer 0
        PendingRPC p; p.peerId=peerId; p.name=name; p.payload=args.data;
        std::lock_guard<std::mutex> g(mutex); incoming.push(p);
    }
};

RPCSystem::RPCSystem() : m_impl(new Impl()) {}
RPCSystem::~RPCSystem() { delete m_impl; }
void RPCSystem::Init()     {}
void RPCSystem::Shutdown() {}

void RPCSystem::Register(const std::string& name, RPCHandler handler){ m_impl->handlers[name]=std::move(handler); }
void RPCSystem::Unregister(const std::string& name){ m_impl->handlers.erase(name); }
bool RPCSystem::IsRegistered(const std::string& name) const{ return m_impl->handlers.count(name)>0; }

void RPCSystem::CallServer(const std::string& name, RPCArgs args, RPCMode mode){ m_impl->SendOutgoing(0,name,args,mode); }
void RPCSystem::CallAll(const std::string& name, RPCArgs args, RPCMode mode){ m_impl->SendOutgoing(0xFFFFFFFFu,name,args,mode); }
void RPCSystem::CallClient(uint32_t peerId, const std::string& name, RPCArgs args, RPCMode mode){ m_impl->SendOutgoing(peerId,name,args,mode); }

void RPCSystem::Tick(){
    std::lock_guard<std::mutex> g(m_impl->mutex);
    while(!m_impl->incoming.empty()){
        auto pend=m_impl->incoming.front(); m_impl->incoming.pop();
        auto it=m_impl->handlers.find(pend.name);
        if(it==m_impl->handlers.end()){
            if(m_impl->onError) m_impl->onError(pend.name,"No handler registered");
            continue;
        }
        RPCContext ctx; ctx.callerPeerId=pend.peerId; ctx.args.data=pend.payload;
        try { it->second(ctx); }
        catch(const std::exception& e){ if(m_impl->onError) m_impl->onError(pend.name,e.what()); }
    }
}

void RPCSystem::InjectIncoming(uint32_t peerId, const std::string& name,
                                const uint8_t* payload, uint32_t len){
    PendingRPC p; p.peerId=peerId; p.name=name;
    p.payload.assign(payload,payload+len);
    std::lock_guard<std::mutex> g(m_impl->mutex); m_impl->incoming.push(p);
}

uint32_t RPCSystem::PendingCount() const{
    std::lock_guard<std::mutex> g(m_impl->mutex);
    return (uint32_t)m_impl->incoming.size();
}

void RPCSystem::OnDispatchError(std::function<void(const std::string&,const std::string&)> cb){ m_impl->onError=std::move(cb); }

} // namespace Engine
