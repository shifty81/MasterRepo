#pragma once
// Hot Reload Config — live parameter changes during simulation
// Parameters are typed, versioned, and validated before application.
// Changes are gated: only permitted between ticks (never mid-tick).

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <variant>

namespace atlas::sim {

/// Types a config parameter can hold
using ConfigValue = std::variant<int32_t, uint32_t, float, double, bool, std::string>;

/// A single configuration parameter
struct ConfigParam {
    std::string key;
    ConfigValue value;
    ConfigValue defaultValue;
    std::string description;
    uint32_t version = 0;        // Bumped on each change
    bool locked = false;         // If true, cannot be changed at runtime
};

/// Result of a config change attempt
struct ConfigChangeResult {
    bool success = false;
    std::string error;
    std::string key;
    uint32_t newVersion = 0;
};

/// Callback fired when a config parameter changes
using ConfigChangeCallback = std::function<void(const std::string& key, const ConfigValue& oldValue, const ConfigValue& newValue)>;

/// Manages hot-reloadable simulation configuration
class HotReloadConfig {
public:
    HotReloadConfig();

    // --- Parameter Registration ---
    void RegisterParam(const std::string& key, const ConfigValue& defaultValue, const std::string& description = "");
    bool HasParam(const std::string& key) const;
    size_t ParamCount() const;

    // --- Get/Set ---
    ConfigValue GetValue(const std::string& key) const;
    template<typename T> T GetAs(const std::string& key) const;
    ConfigChangeResult SetValue(const std::string& key, const ConfigValue& value);

    // --- Locking ---
    void LockParam(const std::string& key);
    void UnlockParam(const std::string& key);
    bool IsLocked(const std::string& key) const;

    // --- Tick Gating ---
    void BeginTick();
    void EndTick();
    bool IsInTick() const;

    // --- Versioning ---
    uint32_t GetParamVersion(const std::string& key) const;
    uint32_t GetGlobalVersion() const;

    // --- Reset ---
    void ResetToDefault(const std::string& key);
    void ResetAllToDefaults();

    // --- Change tracking ---
    void SetChangeCallback(ConfigChangeCallback callback);
    std::vector<std::string> GetChangedKeys() const;
    void ClearChangedKeys();

    // --- Snapshot ---
    std::vector<ConfigParam> GetAllParams() const;
    const ConfigParam* GetParam(const std::string& key) const;

private:
    std::unordered_map<std::string, ConfigParam> m_params;
    ConfigChangeCallback m_changeCallback;
    std::vector<std::string> m_changedKeys;
    uint32_t m_globalVersion = 0;
    bool m_inTick = false;
};

// Template implementation
template<typename T>
T HotReloadConfig::GetAs(const std::string& key) const {
    auto value = GetValue(key);
    return std::get<T>(value);
}

}  // namespace atlas::sim
