#include "Engine/Vehicle/VehicleSystem/VehicleSystem.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

static float Len3(const float v[3]) {
    return std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
}
static void Norm3(float v[3]) {
    float l=Len3(v)+1e-9f; v[0]/=l;v[1]/=l;v[2]/=l;
}

struct WheelState {
    float spinRpm     {0.f};
    float slipRatio   {0.f};
    bool  grounded    {false};
    float suspOffset  {0.f};   ///< compression (positive = compressed)
};

struct VehicleData {
    uint32_t          id{0};
    VehicleDesc       desc;
    VehicleTransform  transform;
    VehicleInput      input;
    WheelState        wheels[8];
    float             rpm     {800.f};
    int32_t           gear    {1};
    float             speed   {0.f};   ///< m/s
};

struct VehicleSystem::Impl {
    std::vector<VehicleData> vehicles;
    uint32_t nextId{1};
    WheelGroundFn groundFn;

    VehicleData* Find(uint32_t id) {
        for(auto& v:vehicles) if(v.id==id) return &v; return nullptr;
    }
    const VehicleData* Find(uint32_t id) const {
        for(auto& v:vehicles) if(v.id==id) return &v; return nullptr;
    }

    // Sample engine torque from 3-point curve
    float SampleTorque(const VehicleDesc& d, float rpm) const {
        auto& tc = d.torqueCurve;
        if (rpm <= tc[0].rpm) return tc[0].torqueNm;
        if (rpm >= tc[2].rpm) return tc[2].torqueNm;
        if (rpm < tc[1].rpm) {
            float t=(rpm-tc[0].rpm)/(tc[1].rpm-tc[0].rpm+1e-9f);
            return tc[0].torqueNm + t*(tc[1].torqueNm-tc[0].torqueNm);
        }
        float t=(rpm-tc[1].rpm)/(tc[2].rpm-tc[1].rpm+1e-9f);
        return tc[1].torqueNm + t*(tc[2].torqueNm-tc[1].torqueNm);
    }
};

VehicleSystem::VehicleSystem()  : m_impl(new Impl) {}
VehicleSystem::~VehicleSystem() { Shutdown(); delete m_impl; }

void VehicleSystem::Init()     {}
void VehicleSystem::Shutdown() { m_impl->vehicles.clear(); }

uint32_t VehicleSystem::CreateVehicle(const VehicleDesc& desc, const float startPos[3])
{
    VehicleData vd; vd.id=m_impl->nextId++; vd.desc=desc;
    if (startPos) for(int i=0;i<3;i++) vd.transform.position[i]=startPos[i];
    vd.transform.orientation[3]=1.f; // identity quat
    m_impl->vehicles.push_back(vd);
    return vd.id;
}

void VehicleSystem::DestroyVehicle(uint32_t id) {
    auto& v=m_impl->vehicles;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& vd){return vd.id==id;}),v.end());
}
bool VehicleSystem::HasVehicle(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void VehicleSystem::SetInput(uint32_t id, const VehicleInput& inp) {
    auto* v=m_impl->Find(id); if(v) v->input=inp;
}
void VehicleSystem::SetGear(uint32_t id, int32_t g) {
    auto* v=m_impl->Find(id); if(v) v->gear=g;
}
int32_t VehicleSystem::GetGear(uint32_t id) const {
    const auto* v=m_impl->Find(id); return v?v->gear:0;
}

VehicleTransform VehicleSystem::GetTransform(uint32_t id) const {
    const auto* v=m_impl->Find(id); return v?v->transform:VehicleTransform{};
}
void VehicleSystem::SetTransform(uint32_t id, const VehicleTransform& tr) {
    auto* v=m_impl->Find(id); if(v) v->transform=tr;
}

float VehicleSystem::GetWheelSpinRpm  (uint32_t id, uint32_t wi) const {
    const auto* v=m_impl->Find(id); return (v&&wi<8)?v->wheels[wi].spinRpm:0.f;
}
float VehicleSystem::GetWheelSlipRatio(uint32_t id, uint32_t wi) const {
    const auto* v=m_impl->Find(id); return (v&&wi<8)?v->wheels[wi].slipRatio:0.f;
}
bool  VehicleSystem::WheelGroundContact(uint32_t id, uint32_t wi) const {
    const auto* v=m_impl->Find(id); return (v&&wi<8)&&v->wheels[wi].grounded;
}

void VehicleSystem::SetWheelGroundFn(WheelGroundFn fn) { m_impl->groundFn=fn; }

float VehicleSystem::GetRpm  (uint32_t id) const { const auto* v=m_impl->Find(id); return v?v->rpm:0.f; }
float VehicleSystem::GetSpeed(uint32_t id) const { const auto* v=m_impl->Find(id); return v?v->speed:0.f; }
std::vector<uint32_t> VehicleSystem::GetAll() const {
    std::vector<uint32_t> out; for(auto& v:m_impl->vehicles) out.push_back(v.id); return out;
}

void VehicleSystem::Tick(float dt)
{
    static const float kAirDensity = 1.225f;
    for (auto& vd : m_impl->vehicles) {
        auto& inp  = vd.input;
        auto& desc = vd.desc;
        auto& tr   = vd.transform;

        // Forward direction (simplified: chassis +Z in world space)
        float fwd[3] = {0.f, 0.f, 1.f};

        // Gear ratio
        int32_t g = std::max(0, std::min(vd.gear, (int32_t)desc.gearCount));
        float gearRatio = (g>0) ? desc.gearRatios[g-1] : 0.f;

        // Engine torque
        float torque = m_impl->SampleTorque(desc, vd.rpm) * inp.throttle;
        float driveForce = gearRatio > 0.f ? torque * gearRatio / (desc.wheels[0].radius+1e-9f) : 0.f;

        // Aerodynamic drag
        float v2 = vd.speed * vd.speed;
        float drag = 0.5f * kAirDensity * desc.dragCoeff * desc.frontalArea * v2;
        if (vd.speed < 0.f) drag = -drag;

        // Braking
        float brakeForce = inp.brake * 8000.f;

        // Net force → acceleration
        float netF = driveForce - drag - brakeForce * (vd.speed>0?1.f:-1.f);
        float accel = netF / desc.mass;
        vd.speed += accel * dt;

        // Clamp speed
        vd.speed = std::max(-30.f, std::min(vd.speed, 80.f));

        // Position update
        for (int i=0;i<3;i++) tr.velocity[i]=fwd[i]*vd.speed;
        for (int i=0;i<3;i++) tr.position[i]+=tr.velocity[i]*dt;

        // RPM update
        if (gearRatio > 0.f) {
            float wheelRpm = vd.speed / (desc.wheels[0].radius * 2.f * 3.14159f) * 60.f;
            vd.rpm = std::abs(wheelRpm * gearRatio);
            vd.rpm = std::max(800.f, std::min(vd.rpm, 6500.f));
        }

        // Wheel spin
        for (uint32_t w=0;w<desc.wheelCount;w++) {
            vd.wheels[w].spinRpm = vd.rpm / (gearRatio>0?gearRatio:1.f);
            vd.wheels[w].grounded = true; // simplified
        }
    }
}

} // namespace Engine
