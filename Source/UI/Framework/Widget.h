#pragma once
#include "Core/Math/Vector.h"
#include <memory>
#include <vector>

namespace NF {

/// @brief 2-D axis-aligned rectangle in screen space.
struct Rect {
    float X{0.f};
    float Y{0.f};
    float Width{0.f};
    float Height{0.f};
};

/// @brief Base class for all UI widgets.
///
/// Widgets form a tree; a parent widget propagates Update and Draw calls to
/// its children in insertion order.
class Widget {
public:
    virtual ~Widget() = default;

    /// @brief Set the screen-space bounding rectangle of this widget.
    void SetRect(const Rect& rect);

    /// @brief Return the screen-space bounding rectangle of this widget.
    [[nodiscard]] const Rect& GetRect() const noexcept;

    /// @brief Advance widget state by @p dt seconds.
    /// @param dt Delta time in seconds.
    virtual void Update(float dt);

    /// @brief Render this widget and all of its children.
    virtual void Draw();

    /// @brief Attach a child widget.
    /// @param child Shared ownership transferred to this widget.
    void AddChild(std::shared_ptr<Widget> child);

    /// @brief Return the list of direct children.
    [[nodiscard]] const std::vector<std::shared_ptr<Widget>>& GetChildren() const noexcept;

protected:
    Rect                                  m_Rect{};
    std::vector<std::shared_ptr<Widget>>  m_Children;
};

} // namespace NF
