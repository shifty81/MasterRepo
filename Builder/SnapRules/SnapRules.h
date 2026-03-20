#pragma once
#include "Builder/Parts/PartDef.h"
#include <vector>
#include <string>

namespace Builder {

struct SnapCandidate {
    uint32_t partId = 0;
    uint32_t snapId = 0;
    float    score  = 0.0f;
};

class SnapRules {
public:
    /// Returns true if snap points a and b are compatible.
    bool CanSnap(const SnapPoint& a, const SnapPoint& b) const;

    /// Find all snap points on `part` compatible with `target`.
    std::vector<SnapCandidate> FindCompatibleSnaps(const PartDef& part,
                                                    const SnapPoint& target) const;

    /// Score how well two snap points match (higher = better).
    float ScoreSnap(const SnapPoint& a, const SnapPoint& b) const;

    /// Register bidirectional type compatibility.
    void RegisterCompatibility(const std::string& typeA, const std::string& typeB);

    bool IsCompatible(const std::string& typeA, const std::string& typeB) const;

private:
    struct CompatPair { std::string a, b; };
    std::vector<CompatPair> m_compat;
};

} // namespace Builder
