#pragma once
#include "UIRenderer.h"
#include <string>

namespace atlas::ui {

/// Data-driven theme configuration controlling the visual appearance
/// of all UI elements.  Every colour, spacing, and typography constant
/// is stored here so themes can be hot-reloaded or serialised without
/// recompilation.
struct UIStyle {
    // ---- Background / surface colours ----
    UIColor background        {30,  30,  30,  255};
    UIColor panelBackground   {40,  40,  40,  255};
    UIColor headerBackground  {50,  50,  55,  255};
    UIColor tooltipBackground {25,  25,  25,  240};

    // ---- Text colours ----
    UIColor textPrimary       {220, 220, 220, 255};
    UIColor textSecondary     {160, 160, 160, 255};
    UIColor textDisabled      {100, 100, 100, 255};
    UIColor textHighlight     {255, 255, 255, 255};

    // ---- Accent / interactive colours ----
    UIColor accent            {0,   122, 204, 255};
    UIColor accentHover       {28,  151, 234, 255};
    UIColor accentPressed     {0,   100, 180, 255};

    // ---- Button colours ----
    UIColor buttonNormal      {60,  60,  60,  255};
    UIColor buttonHover       {75,  75,  75,  255};
    UIColor buttonPressed     {45,  45,  45,  255};
    UIColor buttonDisabled    {50,  50,  50,  255};

    // ---- Selection / highlight ----
    UIColor selectionBg       {0,   122, 204, 80};
    UIColor hoverBg           {255, 255, 255, 20};

    // ---- Border colours ----
    UIColor border            {60,  60,  60,  255};
    UIColor borderFocused     {0,   122, 204, 255};

    // ---- Status colours ----
    UIColor statusOk          {78,  201, 176, 255};
    UIColor statusWarning     {230, 180, 34,  255};
    UIColor statusError       {244, 71,  71,  255};

    // ---- Spacing (pixels) ----
    int32_t paddingSmall  = 4;
    int32_t paddingMedium = 8;
    int32_t paddingLarge  = 16;
    int32_t itemSpacing   = 4;
    int32_t sectionGap    = 12;

    // ---- Typography ----
    float   fontSizeSmall   = 12.0f;
    float   fontSizeNormal  = 14.0f;
    float   fontSizeLarge   = 18.0f;
    float   fontSizeHeading = 22.0f;
    int32_t lineHeight      = 20;

    // ---- Border / rounding ----
    int32_t borderThickness = 1;
    int32_t cornerRadius    = 4;

    // ---- Helpers ----
    static UIStyle Dark();
    static UIStyle Light();
};

} // namespace atlas::ui
