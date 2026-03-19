#include "Core/ResourceManager/ResourceManager.h"

namespace Core::ResourceManager {

void ResourceManager::Unload(const std::string& id) {
    std::lock_guard lock(m_Mutex);
    m_Cache.erase(id);
}

void ResourceManager::UnloadAll() {
    std::lock_guard lock(m_Mutex);
    m_Cache.clear();
}

std::size_t ResourceManager::CacheSize() const {
    std::lock_guard lock(m_Mutex);
    // Sweep expired weak_ptr entries so the returned count reflects live resources.
    const_cast<ResourceManager*>(this)->SweepExpired();
    return m_Cache.size();
}

void ResourceManager::SweepExpired() {
    // Called with m_Mutex already held.
    for (auto it = m_Cache.begin(); it != m_Cache.end(); ) {
        if (it->second.WeakPtr.expired()) {
            it = m_Cache.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace Core::ResourceManager
