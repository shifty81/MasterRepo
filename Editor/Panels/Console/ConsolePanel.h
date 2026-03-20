#pragma once
#include <string>
#include <vector>

namespace Editor {

class ConsolePanel {
public:
    ConsolePanel() = default;

    // IPanel-compatible interface
    std::string Name() const { return "Console"; }
    bool IsVisible() const   { return m_visible; }
    void SetVisible(bool v)  { m_visible = v; }

    void AddLine(const std::string& line);
    const std::vector<std::string>& Lines() const;
    void Clear();
    size_t LineCount() const;

private:
    std::vector<std::string> m_lines;
    bool m_visible = true;
};

} // namespace Editor
