#pragma once
#include "UI/GUISystem/GUISystem.h"
#include <string>
#include <vector>
#include <functional>

namespace Editor {

// ──────────────────────────────────────────────────────────────
// GUI Editor tool — visual designer for in-game GUI layouts
// ──────────────────────────────────────────────────────────────

enum class GUIEditorMode { Select, Move, Resize, Add, Delete };

struct GUIEditorState {
    std::string         activeScreenName;
    UI::GUIElement*     selectedElement = nullptr;
    GUIEditorMode       mode            = GUIEditorMode::Select;
    bool                showGrid        = true;
    float               gridSize        = 8.f;
    bool                snapToGrid      = true;
    float               canvasZoom      = 1.f;
    float               canvasOffsetX   = 0.f;
    float               canvasOffsetY   = 0.f;
};

class GUIEditor {
public:
    // Lifecycle
    void Open(const std::string& layoutPath = "");
    void Close();
    void Update(float deltaTime);
    void Render();

    // Screen / layout management
    void NewScreen(const std::string& name);
    void LoadLayout(const std::string& filePath);
    void SaveLayout(const std::string& filePath) const;
    void SetActiveScreen(const std::string& name);

    // Element spawning (called from toolbar / context menu)
    void AddElement(const std::string& type, const UI::GUIRect& rect = {});

    // Selection
    void SelectElement(UI::GUIElement* elem);
    void DeselectAll();
    void DeleteSelected();
    void DuplicateSelected();

    // Transform
    void MoveSelected(float dx, float dy);
    void ResizeSelected(float dw, float dh);

    // Inspector bindings (editor can display/edit properties)
    using PropertyChangedCallback = std::function<void(UI::GUIElement*)>;
    void OnPropertyChanged(PropertyChangedCallback cb);

    // Undo / redo
    void Undo();
    void Redo();

    // Mode
    void SetMode(GUIEditorMode mode);
    GUIEditorMode GetMode() const { return m_state.mode; }

    // State
    const GUIEditorState& GetState() const { return m_state; }

    // Singleton
    static GUIEditor& Get();

private:
    void SnapToGrid(UI::GUIRect& rect) const;
    void RenderCanvas();
    void RenderInspector();
    void RenderToolbar();
    void RenderScreenList();

    GUIEditorState           m_state;
    UI::GUISystem*           m_guiSystem = nullptr;
    PropertyChangedCallback  m_onPropertyChanged;
    bool                     m_open = false;

    // Command history for undo/redo
    struct Command {
        std::string description;
        std::function<void()> execute;
        std::function<void()> undo;
    };
    std::vector<Command> m_history;
    int                  m_historyPos = -1;
};

} // namespace Editor
