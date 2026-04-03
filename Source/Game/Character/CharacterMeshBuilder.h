#pragma once
#include "Renderer/Mesh/MeshRenderer.h"
#include "Game/Character/CharacterDef.h"

namespace NF::Game {

// ---------------------------------------------------------------------------
// CharacterMeshBuilder
//
// Builds CPU-side MeshData for each body part of the low-poly player
// character.  Each part is one or more axis-aligned boxes; TexCoord.X
// encodes the CharacterColor palette index so the character shader can
// colour each part independently.
//
// Reference silhouette: trench-coat explorer
//   - dark messy hair
//   - military-green full-length trench coat with visible lapels
//   - warm brown inner shirt (peeking through lapels)
//   - dark navy trousers (visible below coat tails)
//   - tan-brown leather boots
//
// All dimensions are in world-space metres.
// Character origin = feet centre (0, 0, 0).
// +Y = up,  +X = right,  -Z = forward.
// Total height ≈ 1.95 m (excluding hair spikes ≈ 2.05 m).
// ---------------------------------------------------------------------------

class CharacterMeshBuilder
{
public:
    // Build a combined mesh for the entire character in one MeshData.
    // Use this for simple rendering where you don't need per-part transforms.
    [[nodiscard]] MeshData BuildFullCharacter() const;

    // Build the mesh for a single named body part.
    // Part names match the boxes defined in BuildFullCharacter.
    [[nodiscard]] MeshData BuildBoots()       const;
    [[nodiscard]] MeshData BuildLegs()        const;
    [[nodiscard]] MeshData BuildCoatTails()   const;
    [[nodiscard]] MeshData BuildCoatBody()    const;
    [[nodiscard]] MeshData BuildCoatCollar()  const;
    [[nodiscard]] MeshData BuildInnerShirt()  const;
    [[nodiscard]] MeshData BuildArms()        const;
    [[nodiscard]] MeshData BuildHands()       const;
    [[nodiscard]] MeshData BuildNeck()        const;
    [[nodiscard]] MeshData BuildHead()        const;
    [[nodiscard]] MeshData BuildHair()        const;

private:
    // Append one axis-aligned box to an existing MeshData.
    // min/max are the AABB corners.
    // colorId is the palette index (TexCoord.X).
    static void AppendBox(MeshData& out,
                          NF::Vector3 min,
                          NF::Vector3 max,
                          CharacterColor color);
};

} // namespace NF::Game
