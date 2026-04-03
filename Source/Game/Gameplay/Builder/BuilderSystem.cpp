#include "Game/Gameplay/Builder/BuilderSystem.h"
#include "Game/Gameplay/Progression/ProgressionSystem.h"
#include "Game/Voxel/VoxelType.h"

namespace NF::Game::Gameplay {

BuilderSystem::BuilderSystem() {
    // Populate default recipes aligned with ResourceType IDs 1..7.
    m_Recipes[0] = { NF::Game::ResourceType::Stone,   1, 1, false };
    m_Recipes[1] = { NF::Game::ResourceType::Ore,     2, 1, false };
    m_Recipes[2] = { NF::Game::ResourceType::Dirt,    1, 1, false };
    m_Recipes[3] = { NF::Game::ResourceType::Rock,    1, 1, false };
    m_Recipes[4] = { NF::Game::ResourceType::Metal,   3, 2, true  }; // AdvancedBuild
    m_Recipes[5] = { NF::Game::ResourceType::Ice,     2, 1, false };
    m_Recipes[6] = { NF::Game::ResourceType::Organic, 2, 1, false };
}

const BuildRecipe* BuilderSystem::FindRecipe(NF::Game::ResourceType type) const noexcept {
    for (const auto& r : m_Recipes) {
        if (r.resource == type) return &r;
    }
    return nullptr;
}

BuildResult BuilderSystem::Build(NF::Game::VoxelEditApi& api,
                                  NF::Game::Inventory&    inv,
                                  int32_t wx, int32_t wy, int32_t wz,
                                  NF::Game::ResourceType type,
                                  ProgressionSystem*     prog)
{
    const BuildRecipe* recipe = FindRecipe(type);
    if (!recipe) return BuildResult::InvalidRecipe;

    if (recipe->requiresAdvancedBuild) {
        if (!prog || !prog->IsUnlocked(SkillUnlock::AdvancedBuild))
            return BuildResult::LockedTier;
    }

    if (!inv.HasItem(type, recipe->cost))
        return BuildResult::InsufficientResources;

    const NF::Game::VoxelId id = NF::Game::ResourceToVoxelId(type);
    const NF::Game::SetResult sr = api.SetVoxel(wx, wy, wz, id);
    if (sr == NF::Game::SetResult::OutOfBounds)
        return BuildResult::OutOfBounds;

    inv.RemoveItem(type, recipe->cost);
    ++m_TotalBuilt;

    if (prog) prog->AddXP(kXpPerBuild);

    return BuildResult::Success;
}

TeardownResult BuilderSystem::Teardown(NF::Game::VoxelEditApi& api,
                                        NF::Game::Inventory&    inv,
                                        int32_t wx, int32_t wy, int32_t wz)
{
    // Query current voxel type before removing it.
    const NF::Game::VoxelId existing = api.GetVoxel(wx, wy, wz);
    if (existing == static_cast<NF::Game::VoxelId>(NF::Game::VoxelType::Air)) {
        // Check if GetVoxel returned 0 because out-of-bounds or because it's actually air.
        // SetVoxel returns OutOfBounds for invalid coords; we'll use Mine to detect.
        const NF::Game::MineReport report = api.Mine(wx, wy, wz);
        if (report.result == NF::Game::MineResult::OutOfBounds)
            return TeardownResult::OutOfBounds;
        return TeardownResult::EmptyCell;
    }

    // Mine the voxel to remove it.
    const NF::Game::MineReport report = api.Mine(wx, wy, wz);
    if (report.result == NF::Game::MineResult::OutOfBounds)
        return TeardownResult::OutOfBounds;

    // Refund resources if we have a recipe for this voxel type.
    const NF::Game::ResourceType resType = NF::Game::VoxelTypeToResource(existing);
    if (resType != NF::Game::ResourceType::None) {
        const BuildRecipe* recipe = FindRecipe(resType);
        if (recipe && recipe->refund > 0)
            inv.AddItem(resType, recipe->refund);
    }

    ++m_TotalTornDown;
    return TeardownResult::Success;
}

} // namespace NF::Game::Gameplay
