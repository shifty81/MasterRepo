#pragma once
#include <string>
#include <vector>

namespace NF::Editor {

/// @brief Panel that displays engine log messages.
class ConsolePanel {
public:
    /// @brief Append a message to the console.
    void AddMessage(std::string msg) { m_Messages.emplace_back(std::move(msg)); }

    /// @brief Remove all messages from the console.
    void Clear() noexcept { m_Messages.clear(); }

    /// @brief Advance panel state.
    void Update(float dt);

    /// @brief Draw the console panel.
    void Draw();

private:
    std::vector<std::string> m_Messages;
};

} // namespace NF::Editor
