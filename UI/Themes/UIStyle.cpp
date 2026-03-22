#include "UI/Themes/UIStyle.h"

namespace UI {

UIStyle UIStyle::Dark() {
    return UIStyle{}; // all defaults are dark-theme values
}

UIStyle UIStyle::Light() {
    UIStyle s;
    s.background        = {245, 245, 245, 255};
    s.panelBackground   = {235, 235, 235, 255};
    s.headerBackground  = {220, 220, 225, 255};
    s.tooltipBackground = {255, 255, 255, 240};
    s.textPrimary       = {30,  30,  30,  255};
    s.textSecondary     = {90,  90,  90,  255};
    s.textDisabled      = {160, 160, 160, 255};
    s.textHighlight     = {0,   0,   0,   255};
    s.accent            = {0,   100, 200, 255};
    s.accentHover       = {20,  130, 230, 255};
    s.accentPressed     = {0,   80,  170, 255};
    s.buttonNormal      = {210, 210, 210, 255};
    s.buttonHover       = {190, 190, 190, 255};
    s.buttonPressed     = {170, 170, 170, 255};
    s.buttonDisabled    = {220, 220, 220, 255};
    s.selectionBg       = {0,   100, 200, 80};
    s.hoverBg           = {0,   0,   0,   20};
    s.border            = {180, 180, 180, 255};
    s.borderFocused     = {0,   100, 200, 255};
    return s;
}

} // namespace UI
