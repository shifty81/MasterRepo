#pragma once
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <functional>
#include <any>
#include <string>
#include <cstring>

namespace atlas::ecs {

using EntityID = uint32_t;
using ComponentTypeID = uint32_t;

struct ComponentData {
    std::vector<uint8_t> data;
    size_t elementSize = 0;
};

// Type-erased serializer for a single component type
struct ComponentSerializer {
    uint32_t typeTag = 0;
    std::function<std::vector<uint8_t>(const std::any&)> serialize;
    std::function<std::any(const uint8_t*, size_t)> deserialize;
};

class World {
public:
    EntityID CreateEntity();
    void DestroyEntity(EntityID id);

    bool IsAlive(EntityID id) const;
    std::vector<EntityID> GetEntities() const;
    size_t EntityCount() const;

    void Update(float dt);

    void SetTickCallback(std::function<void(float)> cb);

    // Component management
    template<typename T>
    void AddComponent(EntityID id, const T& component) {
        auto key = std::type_index(typeid(T));
        m_components[id][key] = component;
    }

    template<typename T>
    T* GetComponent(EntityID id) {
        auto it = m_components.find(id);
        if (it == m_components.end()) return nullptr;
        auto key = std::type_index(typeid(T));
        auto cit = it->second.find(key);
        if (cit == it->second.end()) return nullptr;
        return std::any_cast<T>(&cit->second);
    }

    template<typename T>
    bool HasComponent(EntityID id) const {
        auto it = m_components.find(id);
        if (it == m_components.end()) return false;
        auto key = std::type_index(typeid(T));
        return it->second.find(key) != it->second.end();
    }

    template<typename T>
    void RemoveComponent(EntityID id) {
        auto it = m_components.find(id);
        if (it == m_components.end()) return;
        auto key = std::type_index(typeid(T));
        it->second.erase(key);
    }

    std::vector<std::type_index> GetComponentTypes(EntityID id) const;

    // Component serializer registration (for POD types)
    template<typename T>
    void RegisterComponent(uint32_t typeTag) {
        auto key = std::type_index(typeid(T));
        ComponentSerializer cs;
        cs.typeTag = typeTag;
        cs.serialize = [](const std::any& val) -> std::vector<uint8_t> {
            const T& v = std::any_cast<const T&>(val);
            std::vector<uint8_t> buf(sizeof(T));
            std::memcpy(buf.data(), &v, sizeof(T));
            return buf;
        };
        cs.deserialize = [](const uint8_t* data, size_t size) -> std::any {
            if (size < sizeof(T)) return {};
            T v;
            std::memcpy(&v, data, sizeof(T));
            return v;
        };
        m_serializers[key] = std::move(cs);
    }

    // ECS state serialization (for snapshot/rollback)
    std::vector<uint8_t> Serialize() const;
    bool Deserialize(const std::vector<uint8_t>& data);

    // Query registered serializer info
    bool HasSerializer(std::type_index key) const;
    uint32_t GetTypeTag(std::type_index key) const;

    // Single-component serialization (for replication deltas)
    std::vector<uint8_t> SerializeComponent(EntityID id, std::type_index key) const;
    bool DeserializeComponent(EntityID id, uint32_t typeTag, const uint8_t* data, size_t size);

private:
    EntityID m_nextID = 1;
    std::vector<EntityID> m_entities;
    std::function<void(float)> m_tickCallback;

    // Component storage: entity -> (type -> data)
    std::unordered_map<EntityID, std::unordered_map<std::type_index, std::any>> m_components;

    // Registered component serializers: type_index -> serializer
    std::unordered_map<std::type_index, ComponentSerializer> m_serializers;
};

}
