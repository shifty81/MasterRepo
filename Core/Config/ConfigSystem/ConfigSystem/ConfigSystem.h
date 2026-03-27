#pragma once
/**
 * @file ConfigSystem.h
 * @brief Typed config key/value store with section support, .ini/.json file load/save.
 *
 * Features:
 *   - Sections: hierarchical namespace (e.g., "Graphics", "Audio")
 *   - Types: bool, int, float, string
 *   - Default values: GetInt/GetFloat/... returns default when key missing
 *   - .ini file load/save (section headers [Section], key=value pairs)
 *   - JSON file load/save
 *   - Change callbacks: per-key on-change notifications
 *   - Read-only keys (set once, protected from further writes)
 *   - Bulk export to flat key map ("Section.key" = value)
 *   - Command-line override: ParseCommandLine() applies --Section.key=value
 *   - Hot-reload: reload file and fire callbacks for changed keys
 *
 * Typical usage:
 * @code
 *   ConfigSystem cfg;
 *   cfg.Init();
 *   cfg.LoadIni("Config/settings.ini");
 *   int width  = cfg.GetInt("Graphics", "Width",  1280);
 *   float vol  = cfg.GetFloat("Audio",   "Volume", 1.0f);
 *   cfg.Set("Graphics", "Width", 1920);
 *   cfg.SaveIni("Config/settings.ini");
 *   cfg.OnChange("Graphics.Width", [](){ RecreateWindow(); });
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Core {

class ConfigSystem {
public:
    ConfigSystem();
    ~ConfigSystem();

    void Init    ();
    void Shutdown();

    // File I/O
    bool LoadIni (const std::string& path);
    bool LoadJSON(const std::string& path);
    bool SaveIni (const std::string& path) const;
    bool SaveJSON(const std::string& path) const;
    void Reload  ();   ///< re-read last loaded file

    // Typed getters (return def if key absent)
    bool        GetBool  (const std::string& section, const std::string& key, bool   def=false) const;
    int32_t     GetInt   (const std::string& section, const std::string& key, int32_t def=0)    const;
    float       GetFloat (const std::string& section, const std::string& key, float   def=0.f)  const;
    std::string GetString(const std::string& section, const std::string& key,
                           const std::string& def="")                                            const;

    // Typed setters
    void Set(const std::string& section, const std::string& key, bool        value);
    void Set(const std::string& section, const std::string& key, int32_t     value);
    void Set(const std::string& section, const std::string& key, float       value);
    void Set(const std::string& section, const std::string& key, const std::string& value);

    // Existence / remove
    bool HasKey    (const std::string& section, const std::string& key) const;
    bool HasSection(const std::string& section)                          const;
    void Remove    (const std::string& section, const std::string& key);
    void RemoveSection(const std::string& section);

    // Protection
    void SetReadOnly(const std::string& section, const std::string& key, bool ro=true);

    // Enumeration
    std::vector<std::string> GetSections()                          const;
    std::vector<std::string> GetKeys     (const std::string& section) const;

    // Flat export ("Section.key" → value string)
    std::unordered_map<std::string,std::string> ExportFlat() const;

    // Command-line overrides: argv format --Section.key=value
    void ParseCommandLine(int argc, char** argv);

    // Change callbacks (key = "Section.key")
    uint32_t OnChange   (const std::string& flatKey, std::function<void()> cb);
    void     RemoveCb   (uint32_t cbId);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Core
