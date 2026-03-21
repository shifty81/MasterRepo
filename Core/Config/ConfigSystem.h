#pragma once
/**
 * @file ConfigSystem.h
 * @brief Hierarchical configuration system with INI/JSON support.
 *
 * ConfigSystem loads key-value settings from .ini or .json files and
 * exposes them through a typed Get/Set API.  Layers are stacked:
 *   1. Default values (set in code)
 *   2. Base config file (e.g. engine.ini)
 *   3. Platform override (e.g. linux.ini)
 *   4. User override (e.g. user.ini / user.json)
 *
 * Later layers override earlier ones.  Live-reload watches the files
 * for changes and re-applies deltas without restarting.
 *
 * All keys are hierarchical: "section.key" or "a.b.c".
 */

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <optional>
#include <cstdint>

namespace Core {

enum class ConfigFormat { Auto, INI, JSON };

struct ConfigEntry {
    std::string key;
    std::string value;
    std::string source;  ///< Which file provided this value
    int         layer{0};
};

using ConfigChangedCallback = std::function<void(const std::string& key,
                                                  const std::string& oldVal,
                                                  const std::string& newVal)>;

/// ConfigSystem — multi-layer hierarchical config with live-reload.
class ConfigSystem {
public:
    ConfigSystem();
    ~ConfigSystem();

    // ── loading ───────────────────────────────────────────────
    bool LoadFile(const std::string& path,
                  ConfigFormat fmt = ConfigFormat::Auto,
                  int layer = 0);
    bool LoadString(const std::string& content,
                    ConfigFormat fmt,
                    const std::string& sourceName,
                    int layer = 0);

    // ── defaults ──────────────────────────────────────────────
    void SetDefault(const std::string& key, const std::string& value);

    // ── get / set ─────────────────────────────────────────────
    std::string  GetString (const std::string& key, const std::string& def = "") const;
    int64_t      GetInt    (const std::string& key, int64_t def = 0) const;
    double       GetDouble (const std::string& key, double def = 0.0) const;
    bool         GetBool   (const std::string& key, bool def = false) const;

    void SetString(const std::string& key, const std::string& value, int layer = 99);
    void SetInt   (const std::string& key, int64_t value,            int layer = 99);
    void SetDouble(const std::string& key, double value,             int layer = 99);
    void SetBool  (const std::string& key, bool value,               int layer = 99);

    bool Has(const std::string& key) const;
    void Remove(const std::string& key, int layer = 99);

    // ── enumeration ───────────────────────────────────────────
    std::vector<std::string> Keys(const std::string& prefix = "") const;
    std::vector<ConfigEntry> AllEntries() const;

    // ── persistence ───────────────────────────────────────────
    /// Save the user override layer to file.
    bool SaveLayer(int layer, const std::string& path,
                   ConfigFormat fmt = ConfigFormat::INI) const;

    // ── live reload ───────────────────────────────────────────
    /// Poll for file changes and reload modified files.
    void PollReload();
    void OnChanged(ConfigChangedCallback cb);

    // ── management ────────────────────────────────────────────
    void Clear(int layer = -1);  ///< -1 clears all layers

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Core
