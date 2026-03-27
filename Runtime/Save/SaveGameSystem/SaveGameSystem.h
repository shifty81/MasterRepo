#pragma once
/**
 * @file SaveGameSystem.h
 * @brief Slot-based save/load: serialise key-value state, versioning, checksums.
 *
 * Features:
 *   - OpenSlot(slotId) → bool: select active save slot
 *   - Save() → bool: flush current state to persistent storage
 *   - Load(slotId) → bool: read state from slot
 *   - DeleteSlot(slotId) → bool
 *   - ListSlots(outSlots[]) → uint32_t
 *   - SetString(key, value) / GetString(key, default) → string
 *   - SetInt(key, value) / GetInt(key, default) → int32_t
 *   - SetFloat(key, value) / GetFloat(key, default) → float
 *   - SetBool(key, value) / GetBool(key, default) → bool
 *   - SetBlob(key, data, size) / GetBlob(key, outData[], outSize) → bool
 *   - HasKey(key) → bool
 *   - RemoveKey(key)
 *   - GetVersion() → uint32_t: save-file schema version
 *   - SetVersion(v)
 *   - SetOnSave(cb) / SetOnLoad(cb) / SetOnError(cb)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct SlotInfo {
    uint32_t    slotId;
    std::string name;
    uint64_t    timestamp; ///< Unix epoch seconds
    uint32_t    version;
};

class SaveGameSystem {
public:
    SaveGameSystem();
    ~SaveGameSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Slot management
    bool     OpenSlot  (uint32_t slotId);
    bool     Save      ();
    bool     Load      (uint32_t slotId);
    bool     DeleteSlot(uint32_t slotId);
    uint32_t ListSlots (std::vector<SlotInfo>& out);

    // Key-value store
    void        SetString(const std::string& key, const std::string& val);
    std::string GetString(const std::string& key,
                          const std::string& def = "") const;
    void        SetInt  (const std::string& key, int32_t val);
    int32_t     GetInt  (const std::string& key, int32_t def = 0) const;
    void        SetFloat(const std::string& key, float val);
    float       GetFloat(const std::string& key, float def = 0.f) const;
    void        SetBool (const std::string& key, bool val);
    bool        GetBool (const std::string& key, bool def = false) const;
    void        SetBlob (const std::string& key,
                         const uint8_t* data, uint32_t size);
    bool        GetBlob (const std::string& key,
                         std::vector<uint8_t>& outData) const;

    bool HasKey   (const std::string& key) const;
    void RemoveKey(const std::string& key);

    // Versioning
    void     SetVersion(uint32_t v);
    uint32_t GetVersion() const;

    // Callbacks
    void SetOnSave (std::function<void(uint32_t slotId)> cb);
    void SetOnLoad (std::function<void(uint32_t slotId)> cb);
    void SetOnError(std::function<void(const std::string&)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
