#include "Builder/Collision/CollisionShape.h"

namespace Builder {

CollisionShape CollisionBuilder::BuildForPart(const PartDef& part) {
    CollisionShape shape;
    if (!part.collisionMesh.empty()) {
        shape.type     = CollisionShapeType::Mesh;
        shape.meshPath = part.collisionMesh;
    } else {
        shape.type = CollisionShapeType::Box;
        // Default unit box; real pipeline would derive from mesh bounds.
        shape.box.halfExtents[0] = 0.5f;
        shape.box.halfExtents[1] = 0.5f;
        shape.box.halfExtents[2] = 0.5f;
    }
    return shape;
}

std::vector<CollisionShape> CollisionBuilder::BuildForAssembly(
    const Assembly& assembly, const PartLibrary& library)
{
    std::vector<CollisionShape> out;
    for (size_t i = 0; i < assembly.PartCount(); ++i) {
        // Iterate by brute-force: Assembly exposes parts via GetPart(instanceId).
        // We need to walk instanceIds; use internal knowledge that ids start at 1.
        // A safe approach: rebuild from placed-part list via a public accessor.
        // Since Assembly only exposes GetPart(id), we iterate candidate ids.
        // NOTE: a better API would expose an iterator; accepted limitation here.
        (void)library;
    }
    // Practical implementation: callers pass a list of (instanceId, defId) pairs.
    // Here we produce one box per part count as a placeholder.
    for (size_t i = 0; i < assembly.PartCount(); ++i) {
        CollisionShape s;
        s.type = CollisionShapeType::Box;
        s.box.halfExtents[0] = s.box.halfExtents[1] = s.box.halfExtents[2] = 0.5f;
        out.push_back(s);
    }
    return out;
}

CollisionShape CollisionBuilder::MergeShapes(std::vector<CollisionShape> shapes) {
    CollisionShape compound;
    compound.type = CollisionShapeType::Compound;
    // A compound shape represents the union; sub-shapes stored externally by caller.
    // For mass-centre we compute AABB extents of all boxes/spheres.
    float maxE[3] = {0, 0, 0};
    for (const auto& s : shapes) {
        if (s.type == CollisionShapeType::Box) {
            for (int k = 0; k < 3; ++k)
                if (s.box.halfExtents[k] > maxE[k]) maxE[k] = s.box.halfExtents[k];
        } else if (s.type == CollisionShapeType::Sphere) {
            for (int k = 0; k < 3; ++k)
                if (s.sphere.radius > maxE[k]) maxE[k] = s.sphere.radius;
        }
    }
    compound.box.halfExtents[0] = maxE[0];
    compound.box.halfExtents[1] = maxE[1];
    compound.box.halfExtents[2] = maxE[2];
    return compound;
}

} // namespace Builder
