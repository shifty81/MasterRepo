#include "ECS.h"
#include <algorithm>
#include <cstring>

namespace atlas::ecs {

EntityID World::CreateEntity() {
    EntityID id = m_nextID++;
    m_entities.push_back(id);
    return id;
}

void World::DestroyEntity(EntityID id) {
    m_entities.erase(
        std::remove(m_entities.begin(), m_entities.end(), id),
        m_entities.end()
    );
    m_components.erase(id);
}

bool World::IsAlive(EntityID id) const {
    return std::find(m_entities.begin(), m_entities.end(), id) != m_entities.end();
}

std::vector<EntityID> World::GetEntities() const {
    return m_entities;
}

size_t World::EntityCount() const {
    return m_entities.size();
}

void World::Update(float dt) {
    if (m_tickCallback) {
        m_tickCallback(dt);
    }
}

void World::SetTickCallback(std::function<void(float)> cb) {
    m_tickCallback = std::move(cb);
}

std::vector<std::type_index> World::GetComponentTypes(EntityID id) const {
    std::vector<std::type_index> types;
    auto it = m_components.find(id);
    if (it != m_components.end()) {
        for (const auto& pair : it->second) {
            types.push_back(pair.first);
        }
    }
    return types;
}

bool World::HasSerializer(std::type_index key) const {
    return m_serializers.find(key) != m_serializers.end();
}

uint32_t World::GetTypeTag(std::type_index key) const {
    auto it = m_serializers.find(key);
    if (it == m_serializers.end()) return 0;
    return it->second.typeTag;
}

std::vector<uint8_t> World::SerializeComponent(EntityID id, std::type_index key) const {
    auto eit = m_components.find(id);
    if (eit == m_components.end()) return {};
    auto cit = eit->second.find(key);
    if (cit == eit->second.end()) return {};
    auto sit = m_serializers.find(key);
    if (sit == m_serializers.end()) return {};
    return sit->second.serialize(cit->second);
}

bool World::DeserializeComponent(EntityID id, uint32_t typeTag, const uint8_t* data, size_t size) {
    // Find the serializer matching this typeTag
    for (const auto& [key, ser] : m_serializers) {
        if (ser.typeTag != typeTag) continue;

        auto val = ser.deserialize(data, size);
        if (!val.has_value()) return false;

        // Ensure entity exists
        if (!IsAlive(id)) {
            m_entities.push_back(id);
            if (id >= m_nextID) m_nextID = id + 1;
        }

        m_components[id][key] = std::move(val);
        return true;
    }
    return false;
}

// Binary format:
//   [uint32_t nextID]
//   [uint32_t entityCount]
//   for each entity:
//     [uint32_t entityID]
//     [uint32_t componentCount]  (only serializable components)
//     for each component:
//       [uint32_t typeTag]
//       [uint32_t dataSize]
//       [uint8_t data[dataSize]]

std::vector<uint8_t> World::Serialize() const {
    std::vector<uint8_t> buf;

    auto writeU32 = [&](uint32_t v) {
        size_t pos = buf.size();
        buf.resize(pos + sizeof(uint32_t));
        std::memcpy(buf.data() + pos, &v, sizeof(uint32_t));
    };

    writeU32(m_nextID);
    writeU32(static_cast<uint32_t>(m_entities.size()));

    for (EntityID eid : m_entities) {
        writeU32(eid);

        auto cit = m_components.find(eid);
        if (cit == m_components.end()) {
            writeU32(0);
            continue;
        }

        // Count serializable components
        uint32_t count = 0;
        for (const auto& [typeIdx, val] : cit->second) {
            if (m_serializers.find(typeIdx) != m_serializers.end()) {
                ++count;
            }
        }
        writeU32(count);

        for (const auto& [typeIdx, val] : cit->second) {
            auto sit = m_serializers.find(typeIdx);
            if (sit == m_serializers.end()) continue;

            writeU32(sit->second.typeTag);
            auto data = sit->second.serialize(val);
            writeU32(static_cast<uint32_t>(data.size()));
            size_t pos = buf.size();
            buf.resize(pos + data.size());
            std::memcpy(buf.data() + pos, data.data(), data.size());
        }
    }

    return buf;
}

bool World::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 2 * sizeof(uint32_t)) return false;

    size_t offset = 0;
    auto readU32 = [&](uint32_t& v) -> bool {
        if (offset + sizeof(uint32_t) > data.size()) return false;
        std::memcpy(&v, data.data() + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        return true;
    };

    // Build reverse lookup: typeTag -> (type_index, deserializer)
    std::unordered_map<uint32_t, std::pair<std::type_index, const ComponentSerializer*>> tagLookup;
    for (const auto& [typeIdx, cs] : m_serializers) {
        tagLookup.emplace(cs.typeTag, std::make_pair(typeIdx, &cs));
    }

    uint32_t nextID = 0;
    if (!readU32(nextID)) return false;

    uint32_t entityCount = 0;
    if (!readU32(entityCount)) return false;

    // Clear current state
    m_entities.clear();
    m_components.clear();

    m_nextID = nextID;

    for (uint32_t i = 0; i < entityCount; ++i) {
        uint32_t eid = 0;
        if (!readU32(eid)) return false;
        m_entities.push_back(eid);

        uint32_t compCount = 0;
        if (!readU32(compCount)) return false;

        for (uint32_t j = 0; j < compCount; ++j) {
            uint32_t tag = 0;
            if (!readU32(tag)) return false;
            uint32_t size = 0;
            if (!readU32(size)) return false;

            if (offset + size > data.size()) return false;

            auto tit = tagLookup.find(tag);
            if (tit != tagLookup.end()) {
                auto val = tit->second.second->deserialize(data.data() + offset, size);
                if (val.has_value()) {
                    m_components[eid][tit->second.first] = std::move(val);
                }
            }
            offset += size;
        }
    }

    return true;
}

}
