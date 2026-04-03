#include "Game/Interaction/MiningTool.h"
#include "Game/Interaction/RigState.h"

namespace NF::Game {

MiningResult MiningTool::Use(VoxelEditApi& api, RigState& rig,
                              int32_t wx, int32_t wy, int32_t wz)
{
    MiningResult result{};

    // Consume energy regardless of mine outcome (tool still swings).
    // Return value (sufficient energy available) intentionally unused here.
    [[maybe_unused]] bool hadEnergy = rig.ConsumeEnergy(kEnergyCostPerSwing);

    // Attempt to mine the voxel.
    result.voxelReport = api.Mine(wx, wy, wz);

    // Convert a successfully destroyed voxel into a resource.
    if (result.voxelReport.result == MineResult::Success) {
        const ResourceType resType =
            VoxelTypeToResource(result.voxelReport.extractedType);

        if (resType != ResourceType::None) {
            result.gathered.type  = resType;
            result.gathered.count = kYieldPerVoxel;
        }
    }

    return result;
}

} // namespace NF::Game
