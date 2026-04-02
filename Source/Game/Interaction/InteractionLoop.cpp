#include "Game/Interaction/InteractionLoop.h"

namespace NF::Game {

void InteractionLoop::Tick(float dt) noexcept {
    m_Rig.Tick(dt);
}

void InteractionLoop::Reset() noexcept {
    m_Rig.Reset();
    m_Inventory.Clear();
}

MiningResult InteractionLoop::Mine(int32_t wx, int32_t wy, int32_t wz) {
    if (!m_Api) return {};

    MiningResult result = m_MiningTool.Use(*m_Api, m_Rig, wx, wy, wz);

    // Deposit gathered resources into the inventory (silently ignore if full).
    if (!result.gathered.IsEmpty())
        m_Inventory.AddItem(result.gathered.type, result.gathered.count);

    return result;
}

PlaceResult InteractionLoop::Place(int32_t wx, int32_t wy, int32_t wz,
                                    ResourceType type)
{
    if (!m_Api) return PlaceResult::OutOfBounds;
    if (!m_Inventory.HasItem(type)) return PlaceResult::NoItem;

    const VoxelId id = ResourceToVoxelId(type);
    const SetResult sr = m_Api->SetVoxel(wx, wy, wz, id);

    if (sr == SetResult::OutOfBounds) return PlaceResult::OutOfBounds;

    m_Inventory.RemoveItem(type);
    return PlaceResult::Success;
}

RepairResult InteractionLoop::Repair(int32_t wx, int32_t wy, int32_t wz,
                                      ResourceType type)
{
    if (!m_Api) return RepairResult::OutOfBounds;
    if (!m_Inventory.HasItem(type)) return RepairResult::NoItem;

    const VoxelId id = ResourceToVoxelId(type);
    const SetResult sr = m_Api->Repair(wx, wy, wz, id);

    if (sr == SetResult::OutOfBounds) return RepairResult::OutOfBounds;

    m_Inventory.RemoveItem(type);
    return RepairResult::Success;
}

} // namespace NF::Game
