#include "Editor/Modes/EditorModes.h"
#include <iterator>

namespace Editor {

static const std::string& modeToString(EditorModeType m) {
    static const std::string names[] = {
        "Select", "Place", "Paint", "Sculpt", "Inspect", "Build"
    };
    auto idx = static_cast<int>(m);
    static const int kModeCount = static_cast<int>(std::size(names));
    if (idx < 0 || idx >= kModeCount) {
        static const std::string unknown = "Unknown";
        return unknown;
    }
    return names[idx];
}

void EditorModeController::SetMode(EditorModeType mode) {
    if (mode == m_mode) return;
    EditorModeType prev = m_mode;
    m_mode = mode;
    if (m_callback) m_callback(prev, m_mode);
}

EditorModeType EditorModeController::GetMode() const { return m_mode; }

const std::string& EditorModeController::ModeName() const {
    return modeToString(m_mode);
}

bool EditorModeController::IsMode(EditorModeType mode) const {
    return m_mode == mode;
}

void EditorModeController::SetOnModeChanged(OnModeChanged fn) {
    m_callback = std::move(fn);
}

} // namespace Editor
