#include "Editor/UI/EditorLayout.h"
#include <fstream>
#include <sstream>

namespace Editor {

void EditorLayout::RegisterPanel(IPanel* panel) {
    if (panel) m_panels.push_back(panel);
}

const std::vector<IPanel*>& EditorLayout::Panels() const { return m_panels; }

DockNode& EditorLayout::Root() { return m_root; }

MenuBar& EditorLayout::GetMenuBar() { return m_menuBar; }

bool EditorLayout::SaveToFile(const std::string& path) const {
    std::ofstream out(path);
    if (!out.is_open()) return false;
    out << "{\n";
    out << "  \"panels\": [\n";
    for (size_t i = 0; i < m_panels.size(); ++i) {
        out << "    { \"name\": \"" << m_panels[i]->Name()
            << "\", \"visible\": " << (m_panels[i]->IsVisible() ? "true" : "false") << " }";
        if (i + 1 < m_panels.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n";
    out << "}\n";
    return true;
}

bool EditorLayout::LoadFromFile(const std::string& /*path*/) {
    // Stub: returns true — actual deserialization can be added later
    return true;
}

} // namespace Editor
