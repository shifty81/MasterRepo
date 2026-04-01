#pragma once
#include <any>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace NF {

/// @brief Opaque handle that uniquely identifies a loaded asset.
using AssetHandle = uint64_t;

/// @brief Sentinel representing an invalid / null asset handle.
inline constexpr AssetHandle NullAsset = 0ull;

/// @brief Central registry for loading, caching, and unloading engine assets.
///
/// Assets are identified by a monotonically increasing AssetHandle.  The
/// manager maintains a reference count per handle; GarbageCollect() removes
/// any asset whose reference count has dropped to zero.
class AssetManager {
public:
    /// @brief Load (or retrieve a cached copy of) an asset at the given path.
    /// @tparam T    Asset type (e.g. Texture, Mesh, Sound).
    /// @param path  Filesystem path to the asset source file.
    /// @return Handle that can be passed to Get<T>() and Unload().
    template<typename T>
    AssetHandle Load(const std::filesystem::path& path) {
        const std::string key = path.string();

        // Return cached handle if already loaded.
        auto it = m_PathToHandle.find(key);
        if (it != m_PathToHandle.end()) {
            ++m_RefCounts[it->second];
            return it->second;
        }

        const AssetHandle handle = ++m_NextHandle;
        m_Assets[handle]        = T{};           // default-construct as placeholder
        m_RefCounts[handle]     = 1u;
        m_PathToHandle[key]     = handle;
        m_HandleToPath[handle]  = key;
        return handle;
    }

    /// @brief Retrieve a pointer to the cached asset.
    /// @tparam T       Expected asset type.
    /// @param handle   Handle returned by Load<T>().
    /// @return Pointer to the asset, or nullptr if the handle is invalid.
    template<typename T>
    T* Get(AssetHandle handle) {
        auto it = m_Assets.find(handle);
        if (it == m_Assets.end()) return nullptr;
        return std::any_cast<T>(&it->second);
    }

    /// @brief Decrement the reference count for an asset.
    ///        The asset is not immediately removed; call GarbageCollect() to
    ///        reclaim memory from assets with a zero reference count.
    /// @param handle Handle of the asset to release.
    void Unload(AssetHandle handle);

    /// @brief Remove all assets whose reference count has reached zero.
    void GarbageCollect();

private:
    AssetHandle m_NextHandle{0ull};
    std::unordered_map<AssetHandle, std::any>         m_Assets;
    std::unordered_map<AssetHandle, uint32_t>         m_RefCounts;
    std::unordered_map<std::string,  AssetHandle>     m_PathToHandle;
    std::unordered_map<AssetHandle,  std::string>     m_HandleToPath;
};

} // namespace NF
