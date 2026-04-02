#pragma once
#include <string>
#include <vector>

namespace NF { class UIRenderer; }

namespace NF::Editor {

/// @brief Panel that displays engine log messages.
class ConsolePanel {
public:
    /// @brief Append a message to the console.
    void AddMessage(std::string msg) { m_Messages.emplace_back(std::move(msg)); }

    /// @brief Remove all messages from the console.
    void Clear() noexcept { m_Messages.clear(); }

    /// @brief Set the UIRenderer used for drawing.
    void SetUIRenderer(UIRenderer* r) noexcept { m_Renderer = r; }

    /// @brief Advance panel state.
    void Update(float dt);

    /// @brief Draw the console panel within the given region.
    void Draw(float x, float y, float w, float h);

private:
    std::vector<std::string> m_Messages;
    UIRenderer*              m_Renderer{nullptr};
};

} // namespace NF::Editor
