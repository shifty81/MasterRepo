#include "Engine/Assets/AssetManager.h"
#include "Core/Logging/Log.h"
#include <vector>

namespace NF {

void AssetManager::Unload(AssetHandle handle) {
    auto it = m_RefCounts.find(handle);
    if (it == m_RefCounts.end()) return;
    if (it->second > 0u)
        --it->second;
}

void AssetManager::GarbageCollect() {
    std::vector<AssetHandle> toRemove;
    for (const auto& [handle, refCount] : m_RefCounts) {
        if (refCount == 0u)
            toRemove.push_back(handle);
    }

    for (AssetHandle handle : toRemove) {
        auto pathIt = m_HandleToPath.find(handle);
        if (pathIt != m_HandleToPath.end()) {
            m_PathToHandle.erase(pathIt->second);
            m_HandleToPath.erase(pathIt);
        }
        m_Assets.erase(handle);
        m_RefCounts.erase(handle);
        Logger::Log(LogLevel::Trace, "Assets",
                    "GC: removed asset handle " + std::to_string(handle));
    }
}

} // namespace NF
