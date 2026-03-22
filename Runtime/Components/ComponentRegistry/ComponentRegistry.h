#pragma once
/**
 * @file ComponentRegistry.h
 * @brief ECS component type registration and metadata system.
 *
 * ComponentRegistry maintains a runtime catalogue of all component types:
 *   - ComponentTypeId: stable 32-bit identifier assigned at registration time.
 *   - ComponentInfo: name, size, alignment, serialisable flag, and a
 *     zero-argument factory function for default-constructing instances.
 *   - Register<T>(name): registers a component type and returns its TypeId.
 *   - TypeId<T>(): retrieve the id for a previously registered type (asserts
 *     if the type was never registered).
 *   - Info(id): look up ComponentInfo by id.
 *   - All(): iterate every registered ComponentInfo.
 *   - Clear(): unregister all types (useful for unit tests).
 *
 * Thread safety:
 *   Registration should occur at startup on the main thread; concurrent reads
 *   via TypeId / Info / All are safe after that point.
 */

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <functional>
#include <typeindex>
#include <unordered_map>

namespace Runtime {

// ── Component type identifier ─────────────────────────────────────────────
using ComponentTypeId = uint32_t;
static constexpr ComponentTypeId kInvalidComponentTypeId = 0;

// ── Component metadata ────────────────────────────────────────────────────
struct ComponentInfo {
    ComponentTypeId id{kInvalidComponentTypeId};
    std::string     name;
    size_t          size{0};        ///< sizeof(T)
    size_t          alignment{0};   ///< alignof(T)
    bool            serialisable{true};
    std::function<void*()> factory; ///< Default-construct a heap instance (caller owns)
};

// ── Registry ──────────────────────────────────────────────────────────────
class ComponentRegistry {
public:
    ComponentRegistry();
    ~ComponentRegistry();

    ComponentRegistry(const ComponentRegistry&) = delete;
    ComponentRegistry& operator=(const ComponentRegistry&) = delete;

    // ── registration ──────────────────────────────────────────
    /**
     * Register component type T.
     * @param name        Human-readable name (e.g. "TransformComponent").
     * @param serialisable Whether instances should be saved/loaded.
     * @returns Stable ComponentTypeId for T.
     */
    template<typename T>
    ComponentTypeId Register(const std::string& name, bool serialisable = true) {
        const std::type_index ti(typeid(T));
        auto it = m_typeMap.find(ti);
        if (it != m_typeMap.end()) return it->second;

        ComponentInfo info;
        info.id          = ++m_nextId;
        info.name        = name;
        info.size        = sizeof(T);
        info.alignment   = alignof(T);
        info.serialisable = serialisable;
        info.factory     = []() -> void* { return new T{}; };

        m_typeMap[ti] = info.id;
        m_infos.push_back(std::move(info));
        return m_infos.back().id;
    }

    /**
     * Look up the ComponentTypeId for a previously registered type.
     * Returns kInvalidComponentTypeId if not registered.
     */
    template<typename T>
    ComponentTypeId TypeId() const {
        const std::type_index ti(typeid(T));
        auto it = m_typeMap.find(ti);
        return it != m_typeMap.end() ? it->second : kInvalidComponentTypeId;
    }

    // ── lookup ────────────────────────────────────────────────
    const ComponentInfo* Info(ComponentTypeId id) const;
    const ComponentInfo* InfoByName(const std::string& name) const;
    bool                 IsRegistered(ComponentTypeId id) const;
    size_t               Count() const;

    // ── iteration ─────────────────────────────────────────────
    const std::vector<ComponentInfo>& All() const;

    // ── factory ───────────────────────────────────────────────
    /// Heap-allocate a default-constructed component instance (caller owns).
    void* Create(ComponentTypeId id) const;

    // ── reset ─────────────────────────────────────────────────
    void Clear();

private:
    std::vector<ComponentInfo>                       m_infos;
    std::unordered_map<std::type_index, ComponentTypeId> m_typeMap;
    ComponentTypeId                                  m_nextId{0};
};

} // namespace Runtime
