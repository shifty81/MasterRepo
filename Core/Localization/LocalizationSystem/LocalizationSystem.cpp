#include "Core/Localization/LocalizationSystem/LocalizationSystem.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace Core {

// ── helpers ───────────────────────────────────────────────────────────────────

static std::string TrimLine(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// ── Impl ─────────────────────────────────────────────────────────────────────

struct LocalizationSystem::Impl {
    std::map<std::string, std::map<std::string, std::string>> locales;
    std::map<std::string, LocaleInfo>                         localeInfo;
    std::string                                               activeLocale;
    std::string                                               defaultLocale;
    std::function<void(const std::string&)>                   onChanged;
};

// ── LocalizationSystem ────────────────────────────────────────────────────────

LocalizationSystem::LocalizationSystem() : m_impl(new Impl{}) {}
LocalizationSystem::~LocalizationSystem() { delete m_impl; }

bool LocalizationSystem::LoadLocale(const std::string& localeId,
                                    const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    std::map<std::string, std::string> entries;
    std::string line;
    while (std::getline(file, line)) {
        line = TrimLine(line);
        if (line.empty() || line[0] == '#') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = TrimLine(line.substr(0, eq));
        std::string val = TrimLine(line.substr(eq + 1));
        if (!key.empty()) entries[key] = val;
    }

    LoadLocaleMap(localeId, entries);
    return true;
}

void LocalizationSystem::LoadLocaleMap(const std::string& localeId,
                                       const std::map<std::string, std::string>& entries) {
    m_impl->locales[localeId] = entries;
    if (m_impl->localeInfo.find(localeId) == m_impl->localeInfo.end()) {
        LocaleInfo info;
        info.id   = localeId;
        info.name = localeId;
        info.rtl  = "ltr";
        m_impl->localeInfo[localeId] = info;
    }
    if (m_impl->defaultLocale.empty()) m_impl->defaultLocale = localeId;
    if (m_impl->activeLocale.empty())  m_impl->activeLocale  = localeId;
}

bool LocalizationSystem::SetLocale(const std::string& localeId) {
    if (m_impl->locales.find(localeId) == m_impl->locales.end()) return false;
    m_impl->activeLocale = localeId;
    if (m_impl->onChanged) m_impl->onChanged(localeId);
    return true;
}

void LocalizationSystem::SetDefaultLocale(const std::string& localeId) {
    m_impl->defaultLocale = localeId;
}

std::string LocalizationSystem::T(const std::string& key) const {
    // Active locale first
    auto it = m_impl->locales.find(m_impl->activeLocale);
    if (it != m_impl->locales.end()) {
        auto k = it->second.find(key);
        if (k != it->second.end()) return k->second;
    }
    // Fallback to default locale
    auto it2 = m_impl->locales.find(m_impl->defaultLocale);
    if (it2 != m_impl->locales.end()) {
        auto k = it2->second.find(key);
        if (k != it2->second.end()) return k->second;
    }
    return key;
}

std::string LocalizationSystem::Tf(const std::string& key, ...) const {
    std::string fmt = T(key);
    va_list args;
    va_start(args, key);
    // Determine required buffer size
    va_list args2;
    va_copy(args2, args);
    int needed = std::vsnprintf(nullptr, 0, fmt.c_str(), args2);
    va_end(args2);
    if (needed < 0) { va_end(args); return fmt; }
    std::string result(static_cast<size_t>(needed), '\0');
    std::vsnprintf(&result[0], static_cast<size_t>(needed) + 1, fmt.c_str(), args);
    va_end(args);
    return result;
}

std::vector<LocaleInfo> LocalizationSystem::AvailableLocales() const {
    std::vector<LocaleInfo> result;
    result.reserve(m_impl->localeInfo.size());
    for (const auto& [id, info] : m_impl->localeInfo) result.push_back(info);
    return result;
}

std::string LocalizationSystem::ActiveLocale() const {
    return m_impl->activeLocale;
}

std::vector<std::string> LocalizationSystem::Keys() const {
    auto it = m_impl->locales.find(m_impl->activeLocale);
    if (it == m_impl->locales.end()) return {};
    std::vector<std::string> keys;
    keys.reserve(it->second.size());
    for (const auto& [k, v] : it->second) keys.push_back(k);
    return keys;
}

void LocalizationSystem::UnloadLocale(const std::string& localeId) {
    m_impl->locales.erase(localeId);
    m_impl->localeInfo.erase(localeId);
    if (m_impl->activeLocale == localeId)  m_impl->activeLocale.clear();
    if (m_impl->defaultLocale == localeId) m_impl->defaultLocale.clear();
}

void LocalizationSystem::Clear() {
    m_impl->locales.clear();
    m_impl->localeInfo.clear();
    m_impl->activeLocale.clear();
    m_impl->defaultLocale.clear();
}

void LocalizationSystem::OnLocaleChanged(
    std::function<void(const std::string&)> callback) {
    m_impl->onChanged = std::move(callback);
}

LocalizationStats LocalizationSystem::Stats() const {
    LocalizationStats s{};
    s.activeLocale = m_impl->activeLocale;

    auto activeIt  = m_impl->locales.find(m_impl->activeLocale);
    auto defaultIt = m_impl->locales.find(m_impl->defaultLocale);

    if (activeIt != m_impl->locales.end()) {
        s.totalKeys = activeIt->second.size();
    }
    if (defaultIt != m_impl->locales.end() &&
        defaultIt->first != m_impl->activeLocale) {
        for (const auto& [k, v] : defaultIt->second) {
            if (activeIt == m_impl->locales.end() ||
                activeIt->second.find(k) == activeIt->second.end()) {
                ++s.missingKeys;
            }
        }
    }
    return s;
}

} // namespace Core
