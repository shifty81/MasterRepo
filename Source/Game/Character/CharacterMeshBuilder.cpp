#include "Game/Character/CharacterMeshBuilder.h"

namespace NF::Game {

// ---------------------------------------------------------------------------
// AppendBox — emit 6 faces (12 triangles) for one AABB
// ---------------------------------------------------------------------------
void CharacterMeshBuilder::AppendBox(MeshData& out,
                                     NF::Vector3 min,
                                     NF::Vector3 max,
                                     CharacterColor color)
{
    const float cid = static_cast<float>(static_cast<int>(color));
    const uint32_t base = static_cast<uint32_t>(out.Vertices.size());

    // Helper: push one quad (4 vertices, 2 triangles).
    // Vertices are specified CCW when viewed from outside the box.
    auto addFace = [&](NF::Vector3 v0, NF::Vector3 v1,
                       NF::Vector3 v2, NF::Vector3 v3,
                       NF::Vector3 normal)
    {
        const uint32_t i = static_cast<uint32_t>(out.Vertices.size());
        out.Vertices.push_back({v0, normal, {cid, 0.f}});
        out.Vertices.push_back({v1, normal, {cid, 0.f}});
        out.Vertices.push_back({v2, normal, {cid, 0.f}});
        out.Vertices.push_back({v3, normal, {cid, 0.f}});
        // Two triangles per quad (CCW winding)
        out.Indices.insert(out.Indices.end(), {i, i+1, i+2, i+2, i+3, i});
    };

    const float x0 = min.X, x1 = max.X;
    const float y0 = min.Y, y1 = max.Y;
    const float z0 = min.Z, z1 = max.Z;

    // -X face
    addFace({x0,y0,z1}, {x0,y1,z1}, {x0,y1,z0}, {x0,y0,z0}, {-1,0,0});
    // +X face
    addFace({x1,y0,z0}, {x1,y1,z0}, {x1,y1,z1}, {x1,y0,z1}, { 1,0,0});
    // -Y face
    addFace({x0,y0,z0}, {x1,y0,z0}, {x1,y0,z1}, {x0,y0,z1}, {0,-1,0});
    // +Y face
    addFace({x0,y1,z1}, {x1,y1,z1}, {x1,y1,z0}, {x0,y1,z0}, {0, 1,0});
    // -Z face
    addFace({x1,y0,z0}, {x0,y0,z0}, {x0,y1,z0}, {x1,y1,z0}, {0,0,-1});
    // +Z face
    addFace({x0,y0,z1}, {x1,y0,z1}, {x1,y1,z1}, {x0,y1,z1}, {0,0, 1});

    (void)base;
}

// ---------------------------------------------------------------------------
// Body-part builders
// All coordinates centred on X=0, feet at Y=0, forward = -Z.
// ---------------------------------------------------------------------------

MeshData CharacterMeshBuilder::BuildBoots() const
{
    MeshData md;
    // Right boot — slightly wider/longer than the leg for the leather look
    AppendBox(md, { 0.04f, 0.00f, -0.15f}, { 0.28f, 0.19f,  0.12f}, CharacterColor::Boots);
    // Left boot
    AppendBox(md, {-0.28f, 0.00f, -0.15f}, {-0.04f, 0.19f,  0.12f}, CharacterColor::Boots);
    return md;
}

MeshData CharacterMeshBuilder::BuildLegs() const
{
    MeshData md;
    // Right lower leg — dark trousers visible below coat tails
    AppendBox(md, { 0.05f, 0.19f, -0.12f}, { 0.23f, 0.72f,  0.10f}, CharacterColor::Pants);
    // Left lower leg
    AppendBox(md, {-0.23f, 0.19f, -0.12f}, {-0.05f, 0.72f,  0.10f}, CharacterColor::Pants);
    // Right upper leg (mostly hidden under coat tails)
    AppendBox(md, { 0.04f, 0.72f, -0.12f}, { 0.22f, 1.12f,  0.10f}, CharacterColor::Pants);
    // Left upper leg
    AppendBox(md, {-0.22f, 0.72f, -0.12f}, {-0.04f, 1.12f,  0.10f}, CharacterColor::Pants);
    return md;
}

MeshData CharacterMeshBuilder::BuildCoatTails() const
{
    MeshData md;
    // Coat tails — the long lower portion of the trench coat
    // Slightly split in the centre back to give the flared duster look
    AppendBox(md, {-0.26f, 0.40f, -0.16f}, { 0.26f, 1.12f,  0.13f}, CharacterColor::CoatGreen);
    // Subtle rear vent / centre split shadow
    AppendBox(md, {-0.03f, 0.40f,  0.10f}, { 0.03f, 1.00f,  0.14f}, CharacterColor::CoatDark);
    return md;
}

MeshData CharacterMeshBuilder::BuildCoatBody() const
{
    MeshData md;
    // Main coat body — torso section
    AppendBox(md, {-0.27f, 1.12f, -0.16f}, { 0.27f, 1.57f,  0.13f}, CharacterColor::CoatGreen);
    // Belt band around the waist
    AppendBox(md, {-0.27f, 1.09f, -0.16f}, { 0.27f, 1.14f,  0.13f}, CharacterColor::Belt);
    return md;
}

MeshData CharacterMeshBuilder::BuildCoatCollar() const
{
    MeshData md;
    // Left lapel / collar piece
    AppendBox(md, {-0.27f, 1.42f, -0.16f}, {-0.02f, 1.60f, -0.02f}, CharacterColor::CoatDark);
    // Right lapel / collar piece
    AppendBox(md, { 0.02f, 1.42f, -0.16f}, { 0.27f, 1.60f, -0.02f}, CharacterColor::CoatDark);
    // Back collar stand
    AppendBox(md, {-0.18f, 1.54f,  0.02f}, { 0.18f, 1.64f,  0.13f}, CharacterColor::CoatDark);
    return md;
}

MeshData CharacterMeshBuilder::BuildInnerShirt() const
{
    MeshData md;
    // Inner shirt visible in the V-opening of the lapels
    AppendBox(md, {-0.09f, 1.25f, -0.15f}, { 0.09f, 1.52f, -0.13f}, CharacterColor::InnerShirt);
    return md;
}

MeshData CharacterMeshBuilder::BuildArms() const
{
    MeshData md;
    // Right upper arm (coat sleeve)
    AppendBox(md, { 0.27f, 1.26f, -0.13f}, { 0.42f, 1.53f,  0.09f}, CharacterColor::CoatGreen);
    // Right lower arm (coat sleeve, slightly narrower)
    AppendBox(md, { 0.28f, 0.94f, -0.11f}, { 0.41f, 1.26f,  0.08f}, CharacterColor::CoatGreen);
    // Left upper arm
    AppendBox(md, {-0.42f, 1.26f, -0.13f}, {-0.27f, 1.53f,  0.09f}, CharacterColor::CoatGreen);
    // Left lower arm
    AppendBox(md, {-0.41f, 0.94f, -0.11f}, {-0.28f, 1.26f,  0.08f}, CharacterColor::CoatGreen);
    return md;
}

MeshData CharacterMeshBuilder::BuildHands() const
{
    MeshData md;
    // Right hand — visible below sleeve cuff
    AppendBox(md, { 0.28f, 0.82f, -0.10f}, { 0.40f, 0.94f,  0.07f}, CharacterColor::Skin);
    // Left hand
    AppendBox(md, {-0.40f, 0.82f, -0.10f}, {-0.28f, 0.94f,  0.07f}, CharacterColor::Skin);
    return md;
}

MeshData CharacterMeshBuilder::BuildNeck() const
{
    MeshData md;
    AppendBox(md, {-0.07f, 1.57f, -0.07f}, { 0.07f, 1.66f,  0.07f}, CharacterColor::Skin);
    return md;
}

MeshData CharacterMeshBuilder::BuildHead() const
{
    MeshData md;
    // Head — slightly angular/blocky for the low-poly look
    AppendBox(md, {-0.14f, 1.66f, -0.13f}, { 0.14f, 1.93f,  0.13f}, CharacterColor::Skin);
    // Face definition: brow ridge (slightly recessed sides)
    AppendBox(md, {-0.13f, 1.79f, -0.14f}, { 0.13f, 1.87f, -0.12f}, CharacterColor::Skin);
    return md;
}

MeshData CharacterMeshBuilder::BuildHair() const
{
    MeshData md;
    // Main hair cap — covers the top and back of head
    AppendBox(md, {-0.14f, 1.90f, -0.11f}, { 0.14f, 2.00f,  0.12f}, CharacterColor::Hair);
    // Side hair — frames the face slightly
    AppendBox(md, {-0.15f, 1.74f,  0.06f}, {-0.13f, 1.91f,  0.12f}, CharacterColor::Hair);
    AppendBox(md, { 0.13f, 1.74f,  0.06f}, { 0.15f, 1.91f,  0.12f}, CharacterColor::Hair);
    // Front spike cluster — messy/spiky look
    AppendBox(md, {-0.07f, 1.97f, -0.10f}, { 0.01f, 2.07f, -0.04f}, CharacterColor::Hair);
    AppendBox(md, { 0.01f, 1.95f, -0.09f}, { 0.08f, 2.05f, -0.03f}, CharacterColor::Hair);
    AppendBox(md, {-0.11f, 1.93f, -0.06f}, {-0.04f, 2.03f, -0.01f}, CharacterColor::Hair);
    // Back tuft
    AppendBox(md, {-0.08f, 1.92f,  0.08f}, { 0.08f, 2.02f,  0.13f}, CharacterColor::Hair);
    return md;
}

// ---------------------------------------------------------------------------
// BuildFullCharacter — combine all parts into a single MeshData
// ---------------------------------------------------------------------------
MeshData CharacterMeshBuilder::BuildFullCharacter() const
{
    MeshData md;

    auto merge = [&](MeshData part)
    {
        const uint32_t indexOffset = static_cast<uint32_t>(md.Vertices.size());
        for (auto& v : part.Vertices)
            md.Vertices.push_back(v);
        for (auto idx : part.Indices)
            md.Indices.push_back(idx + indexOffset);
    };

    merge(BuildBoots());
    merge(BuildLegs());
    merge(BuildCoatTails());
    merge(BuildCoatBody());
    merge(BuildCoatCollar());
    merge(BuildInnerShirt());
    merge(BuildArms());
    merge(BuildHands());
    merge(BuildNeck());
    merge(BuildHead());
    merge(BuildHair());

    return md;
}

} // namespace NF::Game
