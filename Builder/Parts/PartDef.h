#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace Builder {

enum class PartCategory : uint8_t {
    Hull,
    Weapon,
    Engine,
    Shield,
    Power,
    Utility,
    Structural,
    Armor
};

struct SnapPoint {
    uint32_t            id = 0;
    float               localPos[3]    = {0.0f, 0.0f, 0.0f};
    float               localNormal[3] = {0.0f, 0.0f, 1.0f};
    std::vector<std::string> compatibleTypes;
    bool                occupied = false;
};

struct PartStats {
    float mass          = 0.0f;
    float hitpoints     = 100.0f;
    float powerDraw     = 0.0f;
    float powerOutput   = 0.0f;
    float thrust        = 0.0f;
    float shieldRegen   = 0.0f;
    float armorRating   = 0.0f;
};

struct PartDef {
    uint32_t                id = 0;
    std::string             name;
    PartCategory            category = PartCategory::Hull;
    std::string             meshPath;
    std::string             collisionMesh;
    PartStats               stats;
    std::vector<SnapPoint>  snapPoints;
    uint8_t                 tier = 1;
    std::string             description;
};

/// Registry of all known PartDef instances. Thread-hostile; use from one thread.
class PartLibrary {
public:
    /// Register a part definition. Overwrites if id already exists.
    void Register(PartDef def);

    /// Remove a part by id.
    void Remove(uint32_t id);

    /// Look up by id (nullptr if not found).
    PartDef* Get(uint32_t id);
    const PartDef* Get(uint32_t id) const;

    /// Look up by name (nullptr if not found).
    PartDef* GetByName(const std::string& name);
    const PartDef* GetByName(const std::string& name) const;

    /// All parts in the given category.
    std::vector<PartDef> GetByCategory(PartCategory category) const;

    size_t Count() const { return m_parts.size(); }
    void   Clear();

    /// Persist to / restore from a minimal JSON file.
    bool LoadFromFile(const std::string& path);
    bool SaveToFile(const std::string& path) const;

private:
    std::unordered_map<uint32_t, PartDef> m_parts;
};

} // namespace Builder
