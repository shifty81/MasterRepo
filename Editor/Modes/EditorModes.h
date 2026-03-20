#pragma once
#include <functional>
#include <string>

namespace Editor {

enum class EditorModeType { Select, Place, Paint, Sculpt, Inspect, Build };

class EditorModeController {
public:
    EditorModeController() = default;

    void           SetMode(EditorModeType mode);
    EditorModeType GetMode() const;
    const std::string& ModeName() const;
    bool           IsMode(EditorModeType mode) const;

    using OnModeChanged = std::function<void(EditorModeType prev, EditorModeType next)>;
    void SetOnModeChanged(OnModeChanged fn);

private:
    EditorModeType m_mode     = EditorModeType::Select;
    OnModeChanged  m_callback;
    mutable std::string m_cachedName;
};

} // namespace Editor
