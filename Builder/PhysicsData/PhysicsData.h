#pragma once
#include "Builder/Assembly/Assembly.h"
#include "Builder/Parts/PartDef.h"

namespace Builder {

struct RigidBodyData {
    float mass           = 0.0f;
    float centerOfMass[3]= {0.0f, 0.0f, 0.0f};
    float inertiaTensor[9]= {1,0,0, 0,1,0, 0,0,1};
    float dragLinear     = 0.01f;
    float dragAngular    = 0.01f;
};

class PhysicsDataBuilder {
public:
    RigidBodyData ComputeForAssembly(const Assembly& assembly,
                                      const PartLibrary& library);
    float         ComputeMass(const Assembly& assembly,
                               const PartLibrary& library);
    void          ComputeCenterOfMass(const Assembly& assembly,
                                       const PartLibrary& library,
                                       float out[3]);
};

} // namespace Builder
