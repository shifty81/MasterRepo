#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace Builder {

struct AssemblyLink {
    uint32_t fromPartId = 0;
    uint32_t fromSnapId = 0;
    uint32_t toPartId   = 0;
    uint32_t toSnapId   = 0;
    bool     weld       = false;
};

struct PlacedPart {
    uint32_t instanceId = 0;
    uint32_t defId      = 0;
    float    transform[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    std::vector<AssemblyLink> links;
};

class Assembly {
public:
    uint32_t AddPart(uint32_t defId, const float transform[16]);
    void     RemovePart(uint32_t instanceId);

    bool Link(uint32_t fromInstanceId, uint32_t fromSnapId,
              uint32_t toInstanceId,   uint32_t toSnapId, bool weld);
    void Unlink(uint32_t instanceId, uint32_t snapId);

    PlacedPart*       GetPart(uint32_t instanceId);
    const PlacedPart* GetPart(uint32_t instanceId) const;

    std::vector<AssemblyLink> GetLinks(uint32_t instanceId) const;

    size_t PartCount() const { return m_parts.size(); }
    size_t LinkCount() const { return m_linkCount; }

    /// Read-only span of all placed parts (use for iteration instead of ID probing).
    const std::vector<PlacedPart>& GetAllParts() const { return m_parts; }

    // These require the caller to supply per-part mass/hp via a lookup table.
    // Lightweight versions that work only from stored PlacedPart data.
    float CalculateTotalMass(const std::vector<std::pair<uint32_t,float>>& defMasses) const;
    float CalculateTotalHP  (const std::vector<std::pair<uint32_t,float>>& defHP)     const;

    /// Returns false if any snap point is linked more than once.
    bool Validate() const;

    void Clear();

private:
    std::vector<PlacedPart> m_parts;
    size_t   m_linkCount = 0;
    uint32_t m_nextId    = 1;
};

} // namespace Builder
