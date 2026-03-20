#include "Builder/PhysicsData/PhysicsData.h"
#include <cstring>

namespace Builder {

float PhysicsDataBuilder::ComputeMass(const Assembly& assembly, const PartLibrary& library) {
    float total = 0.0f;
    // Iterate candidate instance ids (ids start at 1, max = PartCount()).
    // We walk all possible ids up to a reasonable upper bound.
    for (uint32_t id = 1; id <= static_cast<uint32_t>(assembly.PartCount() * 2 + 1); ++id) {
        const PlacedPart* pp = assembly.GetPart(id);
        if (!pp) continue;
        const PartDef* def = library.Get(pp->defId);
        if (def) total += def->stats.mass;
    }
    return total;
}

void PhysicsDataBuilder::ComputeCenterOfMass(const Assembly& assembly,
                                              const PartLibrary& library,
                                              float out[3]) {
    out[0] = out[1] = out[2] = 0.0f;
    float totalMass = 0.0f;
    for (uint32_t id = 1; id <= static_cast<uint32_t>(assembly.PartCount() * 2 + 1); ++id) {
        const PlacedPart* pp = assembly.GetPart(id);
        if (!pp) continue;
        const PartDef* def = library.Get(pp->defId);
        if (!def || def->stats.mass <= 0.0f) continue;
        float m = def->stats.mass;
        // Translation is stored in column-major transform at indices 12,13,14.
        out[0] += pp->transform[12] * m;
        out[1] += pp->transform[13] * m;
        out[2] += pp->transform[14] * m;
        totalMass += m;
    }
    if (totalMass > 0.0f) {
        out[0] /= totalMass;
        out[1] /= totalMass;
        out[2] /= totalMass;
    }
}

RigidBodyData PhysicsDataBuilder::ComputeForAssembly(const Assembly& assembly,
                                                      const PartLibrary& library) {
    RigidBodyData data;
    data.mass = ComputeMass(assembly, library);
    ComputeCenterOfMass(assembly, library, data.centerOfMass);
    // Identity inertia tensor scaled by mass (box approximation).
    float m = data.mass > 0.0f ? data.mass : 1.0f;
    data.inertiaTensor[0] = m; data.inertiaTensor[4] = m; data.inertiaTensor[8] = m;
    return data;
}

} // namespace Builder
