#pragma once
/**
 * @file VehicleSystem.h
 * @brief Wheeled vehicle physics: chassis, wheels, suspension, steering, throttle/brake.
 *
 * Features:
 *   - Vehicle definition: chassis mass, centre-of-mass, inertia tensor
 *   - Up to 8 wheels per vehicle: position (local), radius, width, friction
 *   - Suspension: spring stiffness, damping, rest length, travel
 *   - Drivetrain: FWD/RWD/AWD, gear ratios, engine torque curve (3 points)
 *   - Inputs: throttle [0,1], brake [0,1], steer [-1,1], handbrake bool
 *   - Wheel spin velocity, slip ratio, lateral slip — simplified Pacejka brush
 *   - Aerodynamics: drag coefficient, downforce coefficient, frontal area
 *   - Tick(dt): Euler integration of chassis velocity + wheel spin
 *   - Multiple vehicles; each assigned a unique ID
 *   - Collision response hooks: SetWheelGroundContactFn
 *   - Output: GetTransform(vehicleId) → position + orientation
 *
 * Typical usage:
 * @code
 *   VehicleSystem vs;
 *   vs.Init();
 *   VehicleDesc d; d.mass=1200.f; d.wheelCount=4;
 *   d.wheels[0]={1.2f,-0.6f,1.6f}; // ... etc
 *   uint32_t id = vs.CreateVehicle(d);
 *   vs.SetInput(id, {0.8f, 0.f, 0.3f, false});
 *   vs.Tick(dt);
 *   auto tr = vs.GetTransform(id);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class Drivetrain : uint8_t { FWD=0, RWD, AWD };

struct WheelDesc {
    float localPos[3]{};      ///< position relative to chassis
    float radius    {0.35f};
    float width     {0.2f};
    float friction  {0.8f};
    bool  driven    {true};   ///< receives drive torque
    bool  steered   {false};
    float maxSteerAngle{30.f}; ///< degrees
};

struct SuspensionDesc {
    float stiffness  {35000.f};  ///< N/m
    float damping    {3000.f};   ///< Ns/m
    float restLength {0.4f};     ///< m
    float travel     {0.15f};    ///< m
};

struct TorquePoint { float rpm{0.f}; float torqueNm{0.f}; };

struct VehicleDesc {
    float       mass            {1200.f};
    float       comOffset       [3]{0,0,0};  ///< centre-of-mass offset
    float       inertiaTensor   [3]{2000,2000,2000}; ///< diagonal
    uint32_t    wheelCount      {4};
    WheelDesc   wheels          [8];
    SuspensionDesc suspension   [8];
    Drivetrain  drivetrain      {Drivetrain::RWD};
    float       gearRatios      [7]{3.6f,2.1f,1.4f,1.f,0.8f,0.f,0.f};
    uint32_t    gearCount       {5};
    TorquePoint torqueCurve     [3]{{800,250},{3500,320},{6000,200}};
    float       dragCoeff       {0.3f};
    float       downforceCoeff  {0.1f};
    float       frontalArea     {2.2f};
};

struct VehicleInput {
    float throttle  {0.f};   ///< 0-1
    float brake     {0.f};   ///< 0-1
    float steer     {0.f};   ///< -1..1
    bool  handbrake {false};
};

struct VehicleTransform {
    float position   [3]{};
    float orientation[4]{0,0,0,1};  ///< quaternion
    float velocity   [3]{};
    float angularVel [3]{};
};

// Called each tick per wheel; returns (normal, depth) or depth<=0 if no contact
using WheelGroundFn = std::function<float(uint32_t vehicleId, uint32_t wheelIdx,
                                          const float worldPos[3], float* outNormal)>;

class VehicleSystem {
public:
    VehicleSystem();
    ~VehicleSystem();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    // Vehicle lifecycle
    uint32_t CreateVehicle (const VehicleDesc& desc, const float startPos[3]=nullptr);
    void     DestroyVehicle(uint32_t id);
    bool     HasVehicle    (uint32_t id) const;

    // Input
    void SetInput    (uint32_t id, const VehicleInput& input);
    void SetGear     (uint32_t id, int32_t gear);   ///< 0=neutral,1-N=forward
    int32_t GetGear  (uint32_t id) const;

    // Transform
    VehicleTransform GetTransform(uint32_t id) const;
    void             SetTransform(uint32_t id, const VehicleTransform& tr);

    // Wheel state
    float GetWheelSpinRpm  (uint32_t id, uint32_t wheelIdx) const;
    float GetWheelSlipRatio(uint32_t id, uint32_t wheelIdx) const;
    bool  WheelGroundContact(uint32_t id, uint32_t wheelIdx) const;

    // Hooks
    void SetWheelGroundFn(WheelGroundFn fn);

    // Engine state
    float GetRpm (uint32_t id) const;
    float GetSpeed(uint32_t id) const;  ///< m/s

    std::vector<uint32_t> GetAll() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
