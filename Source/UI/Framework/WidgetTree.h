#pragma once
#include "UI/Framework/Widget.h"
#include "Core/Math/Vector.h"
#include <memory>

namespace NF {

/// @brief Owns the root widget and drives the entire widget hierarchy.
class WidgetTree {
public:
    /// @brief Replace the root widget.
    /// @param root Shared ownership of the new root.
    void SetRoot(std::shared_ptr<Widget> root);

    /// @brief Propagate Update to the root and its entire subtree.
    /// @param dt Delta time in seconds.
    void Update(float dt);

    /// @brief Propagate Draw to the root and its entire subtree.
    void Draw();

    /// @brief Find the topmost widget whose Rect contains @p pos.
    /// @param pos Screen-space query position.
    /// @return Raw pointer to the hit widget, or nullptr if none.
    [[nodiscard]] Widget* HitTest(const Vector2& pos) const;

private:
    Widget* HitTestNode(Widget* widget, const Vector2& pos) const;

    std::shared_ptr<Widget> m_Root;
};

} // namespace NF
