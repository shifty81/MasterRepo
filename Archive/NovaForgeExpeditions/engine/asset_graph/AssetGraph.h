#pragma once
#include <cstdint>
#include <string>

namespace atlas::asset {

using AssetSeed = uint64_t;

struct AssetContext {
    AssetSeed seed;
    uint32_t lod;
};

class AssetNode {
public:
    virtual ~AssetNode() = default;
    virtual void Evaluate(const AssetContext&) = 0;
    virtual std::string Name() const = 0;
};

} // namespace atlas::asset
