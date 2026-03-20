#include "Builder/SnapRules/SnapRules.h"
#include <cmath>
#include <algorithm>

namespace Builder {

bool SnapRules::IsCompatible(const std::string& typeA, const std::string& typeB) const {
    for (const auto& cp : m_compat)
        if ((cp.a == typeA && cp.b == typeB) || (cp.a == typeB && cp.b == typeA))
            return true;
    return false;
}

void SnapRules::RegisterCompatibility(const std::string& typeA, const std::string& typeB) {
    if (!IsCompatible(typeA, typeB))
        m_compat.push_back({typeA, typeB});
}

bool SnapRules::CanSnap(const SnapPoint& a, const SnapPoint& b) const {
    if (a.occupied || b.occupied) return false;
    for (const auto& ta : a.compatibleTypes)
        for (const auto& tb : b.compatibleTypes)
            if (ta == tb || IsCompatible(ta, tb)) return true;
    return false;
}

float SnapRules::ScoreSnap(const SnapPoint& a, const SnapPoint& b) const {
    if (!CanSnap(a, b)) return 0.0f;
    // Normals should be anti-parallel for a good snap (dot ≈ -1).
    float dot = a.localNormal[0]*b.localNormal[0]
              + a.localNormal[1]*b.localNormal[1]
              + a.localNormal[2]*b.localNormal[2];
    // Map [-1,1] → [1,0]; perfect anti-parallel → score 1.
    return std::max(0.0f, (-dot + 1.0f) * 0.5f);
}

std::vector<SnapCandidate> SnapRules::FindCompatibleSnaps(const PartDef& part,
                                                           const SnapPoint& target) const {
    std::vector<SnapCandidate> out;
    for (const auto& sp : part.snapPoints) {
        float s = ScoreSnap(sp, target);
        if (s > 0.0f) out.push_back({part.id, sp.id, s});
    }
    std::sort(out.begin(), out.end(),
        [](const SnapCandidate& a, const SnapCandidate& b){ return a.score > b.score; });
    return out;
}

} // namespace Builder
