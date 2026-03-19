#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace Core::ResourceManager {

/// Type-safe shared ownership handle for a cached resource.
///
/// ResourceHandle wraps a std::shared_ptr<T> and carries the string ID that
/// was used to load the resource.  Multiple handles to the same resource share
/// ownership; the cache holds one weak_ptr so resources can be evicted when
/// all outside handles are released.
///
/// Usage:
///   auto tex = manager.Load<Texture>("tex_player", "assets/player.png");
///   if (tex) { renderer.Draw(*tex); }
template <typename T>
class ResourceHandle {
public:
    ResourceHandle() = default;

    explicit ResourceHandle(std::string id, std::shared_ptr<T> ptr)
        : m_ID(std::move(id)), m_Ptr(std::move(ptr)) {}

    // --- Accessors ---

    [[nodiscard]] const std::string& GetID() const noexcept { return m_ID; }
    [[nodiscard]] T*                 Get()   const noexcept { return m_Ptr.get(); }
    [[nodiscard]] long               UseCount() const noexcept { return m_Ptr.use_count(); }

    [[nodiscard]] bool IsValid() const noexcept { return m_Ptr != nullptr; }
    explicit operator bool() const noexcept { return IsValid(); }

    T&       operator*()        { return *m_Ptr; }
    const T& operator*()  const { return *m_Ptr; }
    T*       operator->()       { return m_Ptr.get(); }
    const T* operator->() const { return m_Ptr.get(); }

private:
    std::string        m_ID;
    std::shared_ptr<T> m_Ptr;
};

// ---------------------------------------------------------------------------
// ResourceLoader — per-type load hook
// ---------------------------------------------------------------------------

/// Type-erased loader interface invoked by ResourceManager to construct a T
/// from a file path.  Specialize or derive to support each resource type.
///
/// Default implementation: T must be constructible from const std::string&
/// (the path).  Override LoadResource() for custom loading logic.
template <typename T>
struct ResourceLoader {
    [[nodiscard]] virtual std::shared_ptr<T> LoadResource(const std::string& path) {
        return std::make_shared<T>(path);
    }
    virtual ~ResourceLoader() = default;
};

// ---------------------------------------------------------------------------
// ResourceManager
// ---------------------------------------------------------------------------

/// Thread-safe resource cache with reference-counted handles.
///
/// Resources are identified by a string ID and stored as weak_ptr so they are
/// automatically evicted when all ResourceHandle owners release their copies.
/// Explicit Unload() forces eviction regardless of outstanding handles.
///
/// Loading is type-parameterised; T must satisfy DefaultConstructible or you
/// must register a custom ResourceLoader<T> before calling Load<T>.
///
/// Usage:
///   Core::ResourceManager::ResourceManager rm;
///
///   auto mesh = rm.Load<Mesh>("mesh_ship", "assets/ship.obj");
///   auto same = rm.Get<Mesh>("mesh_ship"); // returns cached handle
///   rm.Unload("mesh_ship");               // evicts entry
///   rm.UnloadAll();                       // clear entire cache
class ResourceManager {
public:
    ResourceManager()  = default;
    ~ResourceManager() = default;

    ResourceManager(const ResourceManager&)            = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    /// Load a resource of type T identified by @p id from @p path.
    /// If a live handle with the same ID already exists in the cache the
    /// cached resource is returned and @p path is ignored.
    template <typename T>
    [[nodiscard]] ResourceHandle<T> Load(const std::string& id, const std::string& path);

    /// Return a cached handle for @p id, or an invalid handle if not present
    /// (or if the resource was evicted because all outside handles were released).
    template <typename T>
    [[nodiscard]] ResourceHandle<T> Get(const std::string& id);

    /// Remove the cache entry for @p id.  Existing handles remain valid.
    void Unload(const std::string& id);

    /// Remove all cache entries.
    void UnloadAll();

    /// Return the number of entries currently in the cache (including entries
    /// whose underlying resource has been evicted but not yet swept).
    [[nodiscard]] std::size_t CacheSize() const;

private:
    // --- Internal types ---

    struct Entry {
        std::string           Path;
        std::type_index       TypeIdx;
        std::weak_ptr<void>   WeakPtr;
    };

    // --- Helpers ---

    /// Sweep dead (evicted) entries from m_Cache to keep CacheSize() honest.
    void SweepExpired();

    // --- State ---

    mutable std::mutex                          m_Mutex;
    std::unordered_map<std::string, Entry>      m_Cache;
};

// ---------------------------------------------------------------------------
// Template implementations (must live in header)
// ---------------------------------------------------------------------------

template <typename T>
ResourceHandle<T> ResourceManager::Load(const std::string& id, const std::string& path) {
    std::lock_guard lock(m_Mutex);

    auto it = m_Cache.find(id);
    if (it != m_Cache.end()) {
        // Return cached if still alive and the same type.
        if (auto sp = std::static_pointer_cast<T>(it->second.WeakPtr.lock())) {
            return ResourceHandle<T>(id, std::move(sp));
        }
    }

    // Construct via the default loader.
    ResourceLoader<T> loader;
    auto sp = loader.LoadResource(path);
    if (!sp) { return {}; }

    m_Cache[id] = Entry{ path, std::type_index(typeid(T)), sp };

    // Periodically sweep dead entries to prevent unbounded cache growth.
    SweepExpired();

    return ResourceHandle<T>(id, std::move(sp));
}

template <typename T>
ResourceHandle<T> ResourceManager::Get(const std::string& id) {
    std::lock_guard lock(m_Mutex);

    auto it = m_Cache.find(id);
    if (it == m_Cache.end()) { return {}; }

    if (auto sp = std::static_pointer_cast<T>(it->second.WeakPtr.lock())) {
        return ResourceHandle<T>(id, std::move(sp));
    }
    return {};
}

} // namespace Core::ResourceManager
