#pragma once
#include <string>
#include "Core/Reflection/Reflect.h"

namespace Core {

struct Person {
    std::string name;
    int age = 0;
    float height = 0.0f;
};

} // namespace Core

// Register Person with the reflection system
REFLECT_BEGIN(Core::Person)
    REFLECT_PROPERTY(name, std::string)
    REFLECT_PROPERTY(age, int)
    REFLECT_PROPERTY(height, float)
REFLECT_END()
