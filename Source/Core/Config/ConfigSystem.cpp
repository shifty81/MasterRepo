#include "Core/Config/ConfigSystem.h"
#include <fstream>

namespace NF {

namespace {
    void TrimRight(std::string& s) {
        auto it = s.find_last_not_of(" \t\r\n");
        if (it != std::string::npos)
            s.erase(it + 1);
        else
            s.clear();
    }

    void TrimLeft(std::string& s) {
        auto it = s.find_first_not_of(" \t");
        if (it != std::string::npos)
            s.erase(0, it);
        else
            s.clear();
    }
} // anonymous namespace

bool ConfigSystem::Load(std::string_view path) {
    const std::string p{path};
    std::ifstream f{p};
    if (!f)
        return false;

    m_Store.clear();
    std::string line;

    while (std::getline(f, line)) {
        TrimLeft(line);
        if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[')
            continue;

        const auto eq = line.find('=');
        if (eq == std::string::npos)
            continue;

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        TrimRight(key);
        TrimLeft(val);
        TrimRight(val);

        if (!key.empty())
            m_Store[key] = val;
    }

    return true;
}

bool ConfigSystem::Save(std::string_view path) const {
    const std::string p{path};
    std::ofstream f{p};
    if (!f)
        return false;

    for (const auto& [key, val] : m_Store)
        f << key << " = " << val << '\n';

    return f.good();
}

} // namespace NF
