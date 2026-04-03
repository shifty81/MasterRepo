#include "Game/Gameplay/Mining/MiningSystem.h"
#include "Game/Gameplay/Progression/ProgressionSystem.h"

namespace NF::Game::Gameplay {

// Tier stat tables -------------------------------------------------------

static constexpr float    kEnergyCosts[]   = { 5.f,  3.5f,  2.f  };
static constexpr uint32_t kYieldMultipliers[] = { 1u,    2u,    4u   };

float MiningSystem::GetEnergyCost() const noexcept {
    return kEnergyCosts[static_cast<int>(m_Tier)];
}

uint32_t MiningSystem::GetYieldMultiplier() const noexcept {
    return kYieldMultipliers[static_cast<int>(m_Tier)];
}

void MiningSystem::SyncTier(const ProgressionSystem& prog) noexcept {
    if (prog.IsUnlocked(SkillUnlock::Tier3Mining))
        m_Tier = MiningTier::Tier3;
    else if (prog.IsUnlocked(SkillUnlock::Tier2Mining))
        m_Tier = MiningTier::Tier2;
    else
        m_Tier = MiningTier::Tier1;
}

NF::Game::MiningResult MiningSystem::Mine(NF::Game::VoxelEditApi& api,
                                           NF::Game::RigState&     rig,
                                           int32_t wx, int32_t wy, int32_t wz,
                                           ProgressionSystem* prog)
{
    // Temporarily override the static energy cost in MiningTool by consuming
    // the delta ourselves before calling the base tool so the rig deducts the
    // tier-specific cost.  MiningTool always deducts kEnergyCostPerSwing.
    // We compensate: if tier cost < base cost, pre-restore the difference.
    using NF::Game::MiningTool;
    const float baseCost = MiningTool::kEnergyCostPerSwing;
    const float tierCost = GetEnergyCost();
    if (tierCost < baseCost)
        rig.RechargeEnergy(baseCost - tierCost);

    NF::Game::MiningResult result = m_Tool.Use(api, rig, wx, wy, wz);

    if (result.voxelReport.result == NF::Game::MineResult::Success) {
        ++m_TotalMined;

        // Scale yield by tier multiplier.
        if (!result.gathered.IsEmpty()) {
            result.gathered.count *= GetYieldMultiplier();
        }

        // Award XP.
        if (prog) {
            prog->AddXP(kXpPerMine);
            SyncTier(*prog);
        }
    }

    return result;
}

} // namespace NF::Game::Gameplay
