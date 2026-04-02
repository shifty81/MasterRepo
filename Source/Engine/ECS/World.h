#pragma once
#include <any>
#include <cstdint>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace NF {

/// @brief Opaque identifier for a live entity inside a World.
using EntityId = uint32_t;

/// @brief Sentinel representing an invalid / null entity.
inline constexpr EntityId NullEntity = 0u;

/// @brief Container for all entities and their components in a scene.
///
/// Components are stored in per-type maps keyed by EntityId.  This is not the
/// most cache-friendly layout but keeps the implementation portable and
/// dependency-free; it can be swapped for a sparse-set or archetype design
/// later without changing the public API.
class World {
public:
    /// @brief Create a new, empty entity and return its identifier.
    EntityId CreateEntity();

    /// @brief Destroy an entity and remove all components attached to it.
    /// @param id The entity to destroy; silently ignored if already destroyed.
    void DestroyEntity(EntityId id);

    /// @brief Attach a component of type T to an entity.
    /// @tparam T   Component type (copyable or movable).
    /// @param id   Target entity.
    /// @param comp Component value to store (forwarded).
    template<typename T>
    void AddComponent(EntityId id, T comp) {
        ComponentStorage<T>()[id] = std::move(comp);
    }

    /// @brief Retrieve a reference to the component of type T on an entity.
    /// @tparam T  Component type.
    /// @param id  Entity that owns the component.
    /// @return Reference to the stored component.
    /// @note Behaviour is undefined if the entity does not have a T component.
    template<typename T>
    T& GetComponent(EntityId id) {
        return ComponentStorage<T>().at(id);
    }

    /// @copydoc GetComponent
    template<typename T>
    const T& GetComponent(EntityId id) const {
        return ComponentStorage<T>().at(id);
    }

    /// @brief Check whether an entity currently has a component of type T.
    /// @tparam T  Component type.
    /// @param id  Entity to query.
    template<typename T>
    bool HasComponent(EntityId id) const {
        auto it = m_Components.find(std::type_index(typeid(T)));
        if (it == m_Components.end()) return false;
        const auto& map = std::any_cast<const std::unordered_map<EntityId, T>&>(it->second);
        return map.count(id) != 0;
    }

    /// @brief Return all (EntityId, T&) pairs for entities that own a T component.
    /// @tparam T  Component type.
    /// @return Vector of pairs; the references remain valid until the next
    ///         structural change (AddComponent / DestroyEntity).
    template<typename T>
    std::vector<std::pair<EntityId, T*>> View() {
        std::vector<std::pair<EntityId, T*>> result;
        auto it = m_Components.find(std::type_index(typeid(T)));
        if (it == m_Components.end()) return result;
        auto& map = std::any_cast<std::unordered_map<EntityId, T>&>(it->second);
        result.reserve(map.size());
        for (auto& [id, comp] : map)
            result.emplace_back(id, &comp);
        return result;
    }

    /// @brief Return the list of all live entity identifiers.
    [[nodiscard]] const std::vector<EntityId>& GetLiveEntities() const noexcept {
        return m_LiveEntities;
    }

private:
    template<typename T>
    std::unordered_map<EntityId, T>& ComponentStorage() {
        auto key = std::type_index(typeid(T));
        auto it  = m_Components.find(key);
        if (it == m_Components.end()) {
            m_Components.emplace(key, std::unordered_map<EntityId, T>{});
            it = m_Components.find(key);
            // Register a type-erased eraser so DestroyEntity can clean up.
            m_Erasers[key] = [this, key](EntityId eid) {
                auto& map = std::any_cast<std::unordered_map<EntityId, T>&>(
                    m_Components.at(key));
                map.erase(eid);
            };
        }
        return std::any_cast<std::unordered_map<EntityId, T>&>(it->second);
    }

    EntityId m_NextId{1u};
    std::vector<EntityId> m_LiveEntities;
    std::unordered_map<std::type_index, std::any> m_Components;
    // Per-type erasure functions registered by AddComponent<T>.
    std::unordered_map<std::type_index, std::function<void(EntityId)>> m_Erasers;
};

} // namespace NF
