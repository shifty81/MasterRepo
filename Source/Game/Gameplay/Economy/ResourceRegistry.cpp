#include "Game/Gameplay/Economy/ResourceRegistry.h"
#include <cstring>

namespace NF::Game::Gameplay {

ResourceRegistry::ResourceRegistry() {
    m_Defs[0] = { NF::Game::ResourceType::None,    "None",    0.f,  0.f,  false, false };
    m_Defs[1] = { NF::Game::ResourceType::Stone,   "Stone",   1.f,  0.5f, true,  false };
    m_Defs[2] = { NF::Game::ResourceType::Ore,     "Ore",     5.f,  0.8f, true,  false };
    m_Defs[3] = { NF::Game::ResourceType::Dirt,    "Dirt",    0.5f, 0.4f, true,  false };
    m_Defs[4] = { NF::Game::ResourceType::Rock,    "Rock",    2.f,  1.2f, true,  false };
    m_Defs[5] = { NF::Game::ResourceType::Metal,   "Metal",  10.f,  2.0f, true,  true  };
    m_Defs[6] = { NF::Game::ResourceType::Ice,     "Ice",     3.f,  0.3f, true,  false };
    m_Defs[7] = { NF::Game::ResourceType::Organic, "Organic", 4.f,  0.6f, true,  true  };
}

const ResourceDef& ResourceRegistry::Get(NF::Game::ResourceType type) const noexcept {
    const int idx = static_cast<int>(type);
    if (idx < 0 || idx >= kCount) return m_Defs[0];
    return m_Defs[static_cast<size_t>(idx)];
}

const ResourceDef& ResourceRegistry::FindByName(const char* name) const noexcept {
    for (const auto& d : m_Defs) {
        if (std::strcmp(d.name, name) == 0) return d;
    }
    return m_Defs[0]; // fallback to None
}

} // namespace NF::Game::Gameplay
