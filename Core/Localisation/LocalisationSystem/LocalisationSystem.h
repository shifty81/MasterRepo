#pragma once
/**
 * @file LocalisationSystem.h
 * @brief String table: load key→value CSV/JSON, locale switch, plural forms, fallback.
 *
 * Features:
 *   - Locale id: e.g. "en", "fr", "de", "ja"
 *   - Key-value string table per locale
 *   - Load from CSV (key,value) or JSON {"key":"value"}
 *   - GetString(key) → returns localised string for current locale
 *   - Falls back to fallback locale if key missing
 *   - Plural forms: GetPlural(key, n) → selects singular/plural variant
 *   - Format: FormatString(key, {{"name","Player"}}) → replaces {name} tokens
 *   - SetLocale / GetLocale
 *   - GetAvailableLocales()
 *   - On-locale-changed callback
 *   - Hot-reload: ReloadLocale(locale)
 *
 * Typical usage:
 * @code
 *   LocalisationSystem ls;
 *   ls.Init("en");
 *   ls.LoadCSV("en", "strings/en.csv");
 *   ls.LoadCSV("fr", "strings/fr.csv");
 *   ls.SetLocale("fr");
 *   auto s = ls.GetString("menu.start");
 *   auto p = ls.GetPlural("items.count", 3); // "3 items"
 *   auto f = ls.FormatString("greeting", {{"name","Alice"}});
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Core {

class LocalisationSystem {
public:
    LocalisationSystem();
    ~LocalisationSystem();

    void Init    (const std::string& fallbackLocale="en");
    void Shutdown();

    // Locale management
    void        SetLocale           (const std::string& locale);
    std::string GetLocale           () const;
    std::string GetFallbackLocale   () const;
    void        SetFallbackLocale   (const std::string& locale);
    std::vector<std::string> GetAvailableLocales() const;

    // Loading
    bool LoadCSV (const std::string& locale, const std::string& path);
    bool LoadJSON(const std::string& locale, const std::string& path);
    void Set     (const std::string& locale, const std::string& key,
                   const std::string& value);

    // Query
    std::string GetString(const std::string& key)         const;
    std::string GetString(const std::string& locale,
                           const std::string& key)         const;
    bool        HasKey   (const std::string& key)         const;

    // Plural forms (singular variant at index 0, plural at index 1)
    void        SetPluralKey(const std::string& locale,
                              const std::string& key,
                              const std::vector<std::string>& forms);
    std::string GetPlural   (const std::string& key, int64_t n) const;

    // Format: replaces {varName} tokens
    std::string FormatString(const std::string& key,
                               const std::unordered_map<std::string,std::string>& vars) const;

    // Reload
    bool ReloadLocale(const std::string& locale);

    // Callback
    void SetOnLocaleChanged(std::function<void(const std::string& oldLocale,
                                                const std::string& newLocale)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Core
