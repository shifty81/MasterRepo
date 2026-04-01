#include "Animation/Skeleton/Skeleton.h"
#include <stdexcept>

namespace NF {

void Skeleton::AddBone(Bone bone) {
    m_Bones.push_back(std::move(bone));
}

int32_t Skeleton::GetBoneIndex(std::string_view name) const {
    for (int32_t i = 0; i < static_cast<int32_t>(m_Bones.size()); ++i) {
        if (m_Bones[i].Name == name)
            return i;
    }
    return -1;
}

const Bone& Skeleton::GetBone(int32_t index) const {
    if (index < 0 || index >= static_cast<int32_t>(m_Bones.size()))
        throw std::out_of_range("Skeleton::GetBone: index out of range");
    return m_Bones[static_cast<std::size_t>(index)];
}

int32_t Skeleton::BoneCount() const noexcept {
    return static_cast<int32_t>(m_Bones.size());
}

} // namespace NF
