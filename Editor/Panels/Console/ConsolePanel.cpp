#include "Editor/Panels/Console/ConsolePanel.h"

namespace Editor {

void ConsolePanel::AddLine(const std::string& line) { m_lines.push_back(line); }
const std::vector<std::string>& ConsolePanel::Lines() const { return m_lines; }
void ConsolePanel::Clear() { m_lines.clear(); }
size_t ConsolePanel::LineCount() const { return m_lines.size(); }

} // namespace Editor
