#include "UI/Localization/Localization.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace UI {

// ── language management ────────────────────────────────────────

void Localization::SetLanguage(const std::string& lang) {
    m_language = lang;
    if (m_onChanged) m_onChanged(lang);
}

void Localization::OnLanguageChanged(LangCallback cb) {
    m_onChanged = std::move(cb);
}

// ── loading ────────────────────────────────────────────────────

bool Localization::LoadFile(const std::string& path, const std::string& lang) {
    std::string useLang = lang.empty() ? m_language : lang;
    // Determine format by extension
    if (path.size() > 4 && path.substr(path.size()-4) == ".csv")
        return LoadCSV(path, useLang);
    return LoadKV(path, useLang);
}

bool Localization::LoadCSV(const std::string& path, const std::string& lang) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto comma = line.find(',');
        if (comma == std::string::npos) continue;
        std::string key = line.substr(0, comma);
        std::string val = line.substr(comma + 1);
        // Strip surrounding quotes if present
        if (val.size() >= 2 && val.front() == '"' && val.back() == '"')
            val = val.substr(1, val.size()-2);
        m_tables[lang][key] = val;
    }
    return true;
}

bool Localization::LoadKV(const std::string& path, const std::string& lang) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        m_tables[lang][key] = val;
    }
    return true;
}

// ── set / get ─────────────────────────────────────────────────

void Localization::Set(const std::string& key, const std::string& value,
                       const std::string& lang) {
    m_tables[lang.empty() ? m_language : lang][key] = value;
}

const std::string& Localization::Get(const std::string& key) const {
    // Try active language
    auto itLang = m_tables.find(m_language);
    if (itLang != m_tables.end()) {
        auto itKey = itLang->second.find(key);
        if (itKey != itLang->second.end()) return itKey->second;
    }
    // Fallback language
    if (m_fallback != m_language) {
        auto itFb = m_tables.find(m_fallback);
        if (itFb != m_tables.end()) {
            auto itKey = itFb->second.find(key);
            if (itKey != itFb->second.end()) return itKey->second;
        }
    }
    return key; // return key itself as last resort
}

std::string Localization::Format(const std::string& key,
                                 const std::vector<std::string>& args) const {
    std::string result = Get(key);
    for (size_t i = 0; i < args.size(); ++i) {
        std::string placeholder = "{" + std::to_string(i) + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.size(), args[i]);
            pos += args[i].size();
        }
    }
    return result;
}

bool Localization::Has(const std::string& key) const {
    auto it = m_tables.find(m_language);
    return it != m_tables.end() && it->second.count(key) > 0;
}

bool Localization::HasLanguage(const std::string& lang) const {
    return m_tables.count(lang) > 0;
}

std::vector<std::string> Localization::Languages() const {
    std::vector<std::string> langs;
    langs.reserve(m_tables.size());
    for (const auto& [lang, _] : m_tables) langs.push_back(lang);
    return langs;
}

// ── singleton ─────────────────────────────────────────────────

Localization& Localization::Instance() {
    static Localization instance;
    return instance;
}

} // namespace UI
