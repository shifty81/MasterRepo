#pragma once
#include <string>
#include <vector>
#include "Engine/Math/Math.h"

namespace Runtime::Components {

struct Transform {
    Engine::Math::Vec3       position{0.0f, 0.0f, 0.0f};
    Engine::Math::Quaternion rotation{0.0f, 0.0f, 0.0f, 1.0f};
    Engine::Math::Vec3       scale{1.0f, 1.0f, 1.0f};

    Engine::Math::Mat4 ToMatrix() const {
        return Engine::Math::Mat4::Translation(position)
             * rotation.ToMatrix()
             * Engine::Math::Mat4::Scale(scale);
    }
};

struct MeshRenderer {
    std::string meshId;
    std::string materialId;
    bool visible    = true;
    bool castShadow = true;
};

struct Light {
    enum class Type { Directional, Point, Spot };
    Type                 type      = Type::Point;
    Engine::Math::Vec3   color{1.0f, 1.0f, 1.0f};
    float                intensity = 1.0f;
    float                range     = 10.0f;
    float                spotAngle = 45.0f;
};

struct CameraComponent {
    float fov      = 60.0f;
    float nearPlane = 0.1f;
    float farPlane  = 1000.0f;
    bool  isMain    = false;
};

struct Collider {
    enum class Shape { Box, Sphere, Capsule, Mesh };
    Shape              shape{Shape::Box};
    Engine::Math::Vec3 extents{1.0f, 1.0f, 1.0f};
    float              radius    = 0.5f;
    float              height    = 2.0f;
    bool               isTrigger = false;
};

struct RigidBody {
    float              mass            = 1.0f;
    Engine::Math::Vec3 velocity{0.0f, 0.0f, 0.0f};
    Engine::Math::Vec3 angularVelocity{0.0f, 0.0f, 0.0f};
    bool               isKinematic = false;
    bool               useGravity  = true;
    float              drag           = 0.0f;
    float              angularDrag    = 0.05f;
};

struct Tag {
    std::string              name;
    std::vector<std::string> tags;
};

} // namespace Runtime::Components
