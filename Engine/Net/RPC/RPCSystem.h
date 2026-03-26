#pragma once
/**
 * @file RPCSystem.h
 * @brief Remote Procedure Call system for multiplayer games.
 *
 * Allows game code to call functions on remote peers or the server with
 * automatic serialisation/deserialisation of arguments.  Supports:
 *   - Server → all clients (broadcast)
 *   - Server → specific client (unicast)
 *   - Client → server
 *   - Reliable (TCP-like) and unreliable (UDP-like) delivery modes
 *   - Argument types: bool, int32, float, string, vec3, byte[]
 *
 * Typical usage:
 * @code
 *   RPCSystem rpc;
 *   rpc.Init();
 *   rpc.Register("SpawnEnemy", [](RPCContext& ctx) {
 *       float pos[3]; ctx.Read(pos);
 *       SpawnEnemy(pos);
 *   });
 *   // On server:
 *   RPCArgs args;
 *   float pos[3]={1,0,3};
 *   args.WriteVec3(pos);
 *   rpc.CallAll("SpawnEnemy", args, RPCMode::Reliable);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class RPCMode : uint8_t { Reliable=0, Unreliable=1 };
enum class RPCTarget : uint8_t { Server=0, AllClients, SpecificClient };

// ── Argument buffer ───────────────────────────────────────────────────────────

struct RPCArgs {
    std::vector<uint8_t> data;

    void WriteBool(bool v);
    void WriteInt32(int32_t v);
    void WriteFloat(float v);
    void WriteString(const std::string& s);
    void WriteVec3(const float v[3]);
    void WriteBytes(const uint8_t* bytes, uint32_t len);

    bool   ReadBool();
    int32_t ReadInt32();
    float   ReadFloat();
    std::string ReadString();
    void   ReadVec3(float out[3]);
    std::vector<uint8_t> ReadBytes();

    void Reset() { data.clear(); readPos=0; }
    size_t readPos{0};
};

// ── RPC call context (receiver side) ─────────────────────────────────────────

struct RPCContext {
    uint32_t callerPeerId{0};
    RPCArgs  args;

    bool   Read(bool& v)         { v=args.ReadBool(); return true; }
    bool   Read(int32_t& v)      { v=args.ReadInt32(); return true; }
    bool   Read(float& v)        { v=args.ReadFloat(); return true; }
    bool   Read(std::string& v)  { v=args.ReadString(); return true; }
    bool   Read(float v[3])      { args.ReadVec3(v); return true; }
};

using RPCHandler = std::function<void(RPCContext&)>;

// ── RPCSystem ─────────────────────────────────────────────────────────────────

class RPCSystem {
public:
    RPCSystem();
    ~RPCSystem();

    void Init();
    void Shutdown();

    // Registration
    void Register(const std::string& name, RPCHandler handler);
    void Unregister(const std::string& name);
    bool IsRegistered(const std::string& name) const;

    // Dispatch (call)
    void CallServer(const std::string& name, RPCArgs args,
                    RPCMode mode = RPCMode::Reliable);
    void CallAll(const std::string& name, RPCArgs args,
                 RPCMode mode = RPCMode::Reliable);
    void CallClient(uint32_t peerId, const std::string& name, RPCArgs args,
                    RPCMode mode = RPCMode::Reliable);

    // Tick – drains incoming queue and dispatches to handlers
    void Tick();

    // Inject incoming raw message (called from network transport layer)
    void InjectIncoming(uint32_t peerId, const std::string& name,
                        const uint8_t* payload, uint32_t payloadLen);

    uint32_t PendingCount() const;

    void OnDispatchError(std::function<void(const std::string& name, const std::string& err)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
