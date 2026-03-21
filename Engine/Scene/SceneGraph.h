#pragma once
#include "Engine/Math/Math.h"
#include <vector>
#include <string>

namespace Engine {

struct SceneNode {
    std::string name;
    Math::Vec3 position;
    Math::Vec3 rotation;
    Math::Vec3 scale{1,1,1};
    std::vector<SceneNode> children;
};

struct Camera {
    Math::Vec3 position{0,0,5};
    Math::Vec3 target{0,0,0};
    Math::Vec3 up{0,1,0};
    float fov = 60.0f;
    float aspect = 16.0f/9.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
};

} // namespace Engine
