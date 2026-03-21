#pragma once
#include <string>
#include "Core/Reflection/Reflect.h"

namespace Core {

struct Entity {
    int id = 0;
    std::string tag;
};

struct Player : public Entity {
    std::string name;
    int level = 1;
};

} // namespace Core

// Register Entity
REFLECT_BEGIN(Core::Entity)
    REFLECT_PROPERTY(id, int)
    REFLECT_PROPERTY(tag, std::string)
REFLECT_END()

// Register Player as derived from Entity
REFLECT_BEGIN_DERIVED(Core::Player, Core::Entity)
    REFLECT_PROPERTY(name, std::string)
    REFLECT_PROPERTY(level, int)
REFLECT_END()
