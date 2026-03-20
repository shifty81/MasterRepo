#include "Builder/Parts/PartDef.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace Builder {

void PartLibrary::Register(PartDef def) {
    uint32_t id = def.id;
    m_parts[id] = std::move(def);
}

void PartLibrary::Remove(uint32_t id) { m_parts.erase(id); }

PartDef* PartLibrary::Get(uint32_t id) {
    auto it = m_parts.find(id);
    return it != m_parts.end() ? &it->second : nullptr;
}
const PartDef* PartLibrary::Get(uint32_t id) const {
    auto it = m_parts.find(id);
    return it != m_parts.end() ? &it->second : nullptr;
}

PartDef* PartLibrary::GetByName(const std::string& name) {
    for (auto& [id, def] : m_parts)
        if (def.name == name) return &def;
    return nullptr;
}
const PartDef* PartLibrary::GetByName(const std::string& name) const {
    for (const auto& [id, def] : m_parts)
        if (def.name == name) return &def;
    return nullptr;
}

std::vector<PartDef> PartLibrary::GetByCategory(PartCategory category) const {
    std::vector<PartDef> out;
    for (const auto& [id, def] : m_parts)
        if (def.category == category) out.push_back(def);
    return out;
}

void PartLibrary::Clear() { m_parts.clear(); }

static std::string Escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else out += c;
    }
    return out;
}

bool PartLibrary::SaveToFile(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << "[\n";
    bool first = true;
    for (const auto& [id, def] : m_parts) {
        if (!first) f << ",\n";
        first = false;
        f << "  {\"id\":" << def.id
          << ",\"name\":\"" << Escape(def.name) << "\""
          << ",\"category\":" << static_cast<int>(def.category)
          << ",\"meshPath\":\"" << Escape(def.meshPath) << "\""
          << ",\"collisionMesh\":\"" << Escape(def.collisionMesh) << "\""
          << ",\"tier\":" << static_cast<int>(def.tier)
          << ",\"description\":\"" << Escape(def.description) << "\""
          << ",\"mass\":" << def.stats.mass
          << ",\"hitpoints\":" << def.stats.hitpoints
          << ",\"powerDraw\":" << def.stats.powerDraw
          << ",\"powerOutput\":" << def.stats.powerOutput
          << ",\"thrust\":" << def.stats.thrust
          << ",\"shieldRegen\":" << def.stats.shieldRegen
          << ",\"armorRating\":" << def.stats.armorRating
          << "}";
    }
    f << "\n]\n";
    return f.good();
}

bool PartLibrary::LoadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::string json((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    auto readVal = [&](const std::string& key, size_t from) -> std::string {
        std::string needle = "\"" + key + "\":";
        size_t p = json.find(needle, from);
        if (p == std::string::npos) return "";
        p += needle.size();
        while (p < json.size() && json[p] == ' ') ++p;
        if (json[p] == '"') {
            ++p;
            std::string v;
            while (p < json.size() && json[p] != '"') {
                if (json[p] == '\\' && p+1 < json.size()) { ++p; }
                v += json[p++];
            }
            return v;
        }
        std::string v;
        while (p < json.size() && json[p] != ',' && json[p] != '}') v += json[p++];
        return v;
    };

    size_t pos = 0;
    while ((pos = json.find('{', pos)) != std::string::npos) {
        PartDef def;
        try {
            def.id            = static_cast<uint32_t>(std::stoul(readVal("id", pos)));
            def.name          = readVal("name", pos);
            def.category      = static_cast<PartCategory>(std::stoi(readVal("category", pos)));
            def.meshPath      = readVal("meshPath", pos);
            def.collisionMesh = readVal("collisionMesh", pos);
            def.tier          = static_cast<uint8_t>(std::stoul(readVal("tier", pos)));
            def.description   = readVal("description", pos);
            def.stats.mass         = std::stof(readVal("mass", pos));
            def.stats.hitpoints    = std::stof(readVal("hitpoints", pos));
            def.stats.powerDraw    = std::stof(readVal("powerDraw", pos));
            def.stats.powerOutput  = std::stof(readVal("powerOutput", pos));
            def.stats.thrust       = std::stof(readVal("thrust", pos));
            def.stats.shieldRegen  = std::stof(readVal("shieldRegen", pos));
            def.stats.armorRating  = std::stof(readVal("armorRating", pos));
            if (def.id != 0) Register(std::move(def));
        } catch (...) {}
        size_t end = json.find('}', pos);
        pos = (end != std::string::npos) ? end + 1 : json.size();
    }
    return true;
}

} // namespace Builder
