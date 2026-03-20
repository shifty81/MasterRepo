#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace UI {

// ──────────────────────────────────────────────────────────────
// Localization — string table with language support
// ──────────────────────────────────────────────────────────────

class Localization {
public:
    // Set active language code, e.g. "en", "fr", "de", "ja"
    void SetLanguage(const std::string& lang);
    const std::string& Language() const { return m_language; }

    // Load a locale file (CSV or simple key=value pairs)
    // CSV format: key,value per line
    // KV format:  key=value per line
    bool LoadFile(const std::string& path, const std::string& lang = "");

    // Register a string directly
    void Set(const std::string& key, const std::string& value,
             const std::string& lang = "");

    // Look up a translated string; returns key if not found
    const std::string& Get(const std::string& key) const;

    // Format string with positional placeholders {0}, {1}, ...
    std::string Format(const std::string& key,
                       const std::vector<std::string>& args) const;

    // Check availability
    bool Has(const std::string& key) const;
    bool HasLanguage(const std::string& lang) const;

    // List all loaded languages
    std::vector<std::string> Languages() const;

    // Callback invoked when the language changes
    using LangCallback = std::function<void(const std::string& newLang)>;
    void OnLanguageChanged(LangCallback cb);

    // Fallback language used when a key is missing in the active lang
    void SetFallback(const std::string& lang) { m_fallback = lang; }

    // Global singleton accessor
    static Localization& Instance();

private:
    using StringTable = std::unordered_map<std::string, std::string>; // key → value
    std::unordered_map<std::string, StringTable> m_tables;             // lang → table
    std::string   m_language = "en";
    std::string   m_fallback = "en";
    LangCallback  m_onChanged;

    bool LoadCSV(const std::string& path, const std::string& lang);
    bool LoadKV (const std::string& path, const std::string& lang);
};

// Convenience shorthand: Loc("key") → Localization::Instance().Get("key")
inline const std::string& Loc(const std::string& key) {
    return Localization::Instance().Get(key);
}

} // namespace UI
