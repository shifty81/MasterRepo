#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace Core {

// ── Data types ────────────────────────────────────────────────────────────────

struct LocaleEntry {
    std::string key;
    std::string value;
};

struct LocaleInfo {
    std::string id;        // e.g. "en_US", "de_DE"
    std::string name;      // e.g. "English (US)"
    std::string rtl;       // "ltr" or "rtl"
};

struct LocalizationStats {
    uint64_t totalKeys{0};
    uint64_t missingKeys{0};   // keys present in default locale but not in current
    std::string activeLocale;
};

// ── LocalizationSystem ────────────────────────────────────────────────────────

class LocalizationSystem {
public:
    LocalizationSystem();
    ~LocalizationSystem();

    // Load a locale from a simple key=value file (UTF-8, one entry per line).
    // Returns true on success.
    bool LoadLocale(const std::string& localeId, const std::string& filePath);

    // Load a locale from an in-memory map.
    void LoadLocaleMap(const std::string& localeId,
                       const std::map<std::string, std::string>& entries);

    // Switch the active locale.  Falls back to the default locale on missing keys.
    bool SetLocale(const std::string& localeId);

    // Set the default / fallback locale (must be loaded first).
    void SetDefaultLocale(const std::string& localeId);

    // Translate a key.  Returns key itself if not found in active or default locale.
    std::string T(const std::string& key) const;

    // Translate with printf-style format arguments.
    std::string Tf(std::string key, ...) const;

    // Registered locale ids.
    std::vector<LocaleInfo> AvailableLocales() const;

    // Active locale id.
    std::string ActiveLocale() const;

    // All keys in the active locale.
    std::vector<std::string> Keys() const;

    // Remove all data for a locale.
    void UnloadLocale(const std::string& localeId);

    // Remove all locales.
    void Clear();

    // Callback fired when the active locale changes.
    void OnLocaleChanged(std::function<void(const std::string& newLocaleId)> callback);

    // Stats.
    LocalizationStats Stats() const;

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Core
