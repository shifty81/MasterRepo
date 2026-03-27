#pragma once
/**
 * @file DialogueWheel.h
 * @brief Radial choice menu: sector layout, hover/select callbacks, icon+text per choice.
 *
 * Features:
 *   - Init(maxChoices): set wheel capacity (3–8 sectors typical)
 *   - SetChoices(choices[]): populate sectors from a DialogueChoice list
 *   - Update(cursorAngleDeg, cursorRadius): update hover state from analog stick or mouse
 *   - GetHoveredIndex() → int: -1 if none, otherwise [0, count)
 *   - GetSelectedIndex() → int: last confirmed selection (-1 if none)
 *   - Confirm(): accept hovered choice, fires OnSelect callback
 *   - Cancel(): dismiss wheel without selection, fires OnCancel callback
 *   - GetSectorAngle(index) → float: centre angle (degrees) of each sector
 *   - GetSectorRect(index, centreX, centreY, radius) → DWRect: bounding rect
 *   - IsVisible() / Show() / Hide()
 *   - SetOnSelect(cb) / SetOnCancel(cb) / SetOnHover(cb)
 *   - SetDeadZoneRadius(r): inner radius threshold (default 0.2 of full radius)
 *   - GetChoiceCount() → uint32_t
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct DWRect    { float x, y, w, h; };

struct DialogueChoice {
    std::string text;
    std::string iconId;   ///< optional icon key
    bool        enabled{true};
};

class DialogueWheel {
public:
    DialogueWheel();
    ~DialogueWheel();

    void Init    (uint32_t maxChoices=8);
    void Shutdown();
    void Reset   ();

    // Content
    void SetChoices(const std::vector<DialogueChoice>& choices);
    void AddChoice (const DialogueChoice& choice);
    void ClearChoices();

    // Per-frame input
    void Update(float cursorAngleDeg, float cursorRadius);

    // State
    int32_t GetHoveredIndex () const;  ///< -1 = none
    int32_t GetSelectedIndex() const;
    bool    IsVisible       () const;

    // Actions
    void Confirm();
    void Cancel ();
    void Show   ();
    void Hide   ();

    // Layout helpers
    float  GetSectorAngle(uint32_t index) const;
    DWRect GetSectorRect (uint32_t index, float cx, float cy, float radius) const;

    // Config
    void SetDeadZoneRadius(float r);   ///< [0,1] fraction of radius
    uint32_t GetChoiceCount() const;

    // Callbacks
    void SetOnSelect(std::function<void(uint32_t index)> cb);
    void SetOnCancel(std::function<void()> cb);
    void SetOnHover (std::function<void(int32_t index)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
