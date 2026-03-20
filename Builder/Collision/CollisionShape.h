#pragma once
#include "Builder/Parts/PartDef.h"
#include "Builder/Assembly/Assembly.h"
#include <string>
#include <vector>

namespace Builder {

enum class CollisionShapeType {
    Box,
    Sphere,
    Capsule,
    Mesh,
    Compound
};

struct CollisionBox    { float halfExtents[3] = {0.5f, 0.5f, 0.5f}; };
struct CollisionSphere { float radius = 0.5f; };
struct CollisionCapsule{ float radius = 0.25f; float halfHeight = 0.5f; };

struct CollisionShape {
    CollisionShapeType type = CollisionShapeType::Box;
    float              transform[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    union {
        CollisionBox     box;
        CollisionSphere  sphere;
        CollisionCapsule capsule;
    };
    std::string meshPath;

    CollisionShape() : box{} {}
};

class CollisionBuilder {
public:
    CollisionShape              BuildForPart(const PartDef& part);
    std::vector<CollisionShape> BuildForAssembly(const Assembly& assembly,
                                                  const PartLibrary& library);
    CollisionShape              MergeShapes(std::vector<CollisionShape> shapes);
};

} // namespace Builder
