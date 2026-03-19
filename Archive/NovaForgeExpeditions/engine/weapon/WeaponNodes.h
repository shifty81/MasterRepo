#pragma once
#include "WeaponGraph.h"

namespace atlas::weapon {

// Base weapon component. Seed input → Component output.
class ReceiverNode : public WeaponNode {
public:
    float baseDamage = 10.0f;
    float baseFireRate = 1.0f;

    const char* GetName() const override { return "Receiver"; }
    const char* GetCategory() const override { return "Component"; }
    std::vector<WeaponPort> Inputs() const override;
    std::vector<WeaponPort> Outputs() const override;
    void Evaluate(const WeaponContext& ctx, const std::vector<WeaponValue>& inputs, std::vector<WeaponValue>& outputs) const override;
};

// Modifies accuracy/range. Component + Float (length) → Component output.
class BarrelNode : public WeaponNode {
public:
    const char* GetName() const override { return "Barrel"; }
    const char* GetCategory() const override { return "Modifier"; }
    std::vector<WeaponPort> Inputs() const override;
    std::vector<WeaponPort> Outputs() const override;
    void Evaluate(const WeaponContext& ctx, const std::vector<WeaponValue>& inputs, std::vector<WeaponValue>& outputs) const override;
};

// Computes recoil from components. Component → Profile output.
class RecoilProfileNode : public WeaponNode {
public:
    const char* GetName() const override { return "RecoilProfile"; }
    const char* GetCategory() const override { return "Profile"; }
    std::vector<WeaponPort> Inputs() const override;
    std::vector<WeaponPort> Outputs() const override;
    void Evaluate(const WeaponContext& ctx, const std::vector<WeaponValue>& inputs, std::vector<WeaponValue>& outputs) const override;
};

// Aggregates final weapon stats. Component + Float (wear) → Stats output.
class WeaponStatsNode : public WeaponNode {
public:
    const char* GetName() const override { return "WeaponStats"; }
    const char* GetCategory() const override { return "Output"; }
    std::vector<WeaponPort> Inputs() const override;
    std::vector<WeaponPort> Outputs() const override;
    void Evaluate(const WeaponContext& ctx, const std::vector<WeaponValue>& inputs, std::vector<WeaponValue>& outputs) const override;
};

}
