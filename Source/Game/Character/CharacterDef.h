#pragma once
#include "Core/Math/Vector.h"

namespace NF::Game {

// ---------------------------------------------------------------------------
// Character appearance colour palette
// Inspired by: low-poly trench-coat explorer aesthetic
// Index is passed via TexCoord.X for the character shader palette lookup.
// ---------------------------------------------------------------------------

enum class CharacterColor : int
{
    Hair        = 0,  // dark charcoal
    Skin        = 1,  // warm tan
    CoatGreen   = 2,  // military green trench coat
    CoatDark    = 3,  // darker green – collar / lapel shadow
    InnerShirt  = 4,  // warm brown inner shirt
    Pants       = 5,  // near-black navy trousers
    Boots       = 6,  // tan-brown leather boots
    Belt        = 7,  // dark leather belt / detail
};

// Returns the world-space RGB for a CharacterColor (used in tools / previews).
inline NF::Vector3 CharacterColorRGB(CharacterColor c) noexcept
{
    switch (c)
    {
    case CharacterColor::Hair:       return {0.15f, 0.12f, 0.10f};
    case CharacterColor::Skin:       return {0.78f, 0.60f, 0.48f};
    case CharacterColor::CoatGreen:  return {0.32f, 0.42f, 0.22f};
    case CharacterColor::CoatDark:   return {0.22f, 0.30f, 0.15f};
    case CharacterColor::InnerShirt: return {0.48f, 0.30f, 0.16f};
    case CharacterColor::Pants:      return {0.12f, 0.12f, 0.18f};
    case CharacterColor::Boots:      return {0.38f, 0.24f, 0.14f};
    case CharacterColor::Belt:       return {0.20f, 0.15f, 0.10f};
    default:                         return {1.0f,  0.0f,  1.0f};
    }
}

} // namespace NF::Game
