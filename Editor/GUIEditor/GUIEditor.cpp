#include "Editor/GUIEditor/GUIEditor.h"
#include <algorithm>
#include <cmath>

namespace Editor {

// ── Singleton ──────────────────────────────────────────────────

GUIEditor& GUIEditor::Get() {
    static GUIEditor instance;
    return instance;
}

// ── Lifecycle ──────────────────────────────────────────────────

void GUIEditor::Open(const std::string& layoutPath) {
    m_guiSystem = &UI::GUISystem::Get();
    m_open      = true;
    if (!layoutPath.empty()) {
        LoadLayout(layoutPath);
    }
}

void GUIEditor::Close() {
    m_open = false;
}

void GUIEditor::Update(float /*deltaTime*/) {
    if (!m_open) return;
    // Tick animations / pending updates
}

void GUIEditor::Render() {
    if (!m_open) return;
    RenderToolbar();
    RenderScreenList();
    RenderCanvas();
    RenderInspector();
}

// ── Screen / layout ────────────────────────────────────────────

void GUIEditor::NewScreen(const std::string& name) {
    if (!m_guiSystem) return;
    m_guiSystem->CreateScreen(name);
    m_state.activeScreenName = name;
    m_state.selectedElement  = nullptr;
}

void GUIEditor::LoadLayout(const std::string& filePath) {
    if (!m_guiSystem) return;
    m_guiSystem->LoadScreenFromJSON(filePath);
}

void GUIEditor::SaveLayout(const std::string& filePath) const {
    if (!m_guiSystem) return;
    m_guiSystem->SaveScreenToJSON(m_state.activeScreenName, filePath);
}

void GUIEditor::SetActiveScreen(const std::string& name) {
    m_state.activeScreenName = name;
    m_state.selectedElement  = nullptr;
}

// ── Element spawning ───────────────────────────────────────────

void GUIEditor::AddElement(const std::string& type, const UI::GUIRect& rect) {
    if (!m_guiSystem || m_state.activeScreenName.empty()) return;
    const std::string& sn = m_state.activeScreenName;
    UI::GUIRect r = rect;
    if (m_state.snapToGrid) SnapToGrid(r);
    static int counter = 0;
    std::string id = type + "_" + std::to_string(++counter);

    if (type == "GUILabel")
        m_guiSystem->AddLabel(sn, id, r, "Label");
    else if (type == "GUIButton")
        m_guiSystem->AddButton(sn, id, r, "Button");
    else if (type == "GUIImage")
        m_guiSystem->AddImage(sn, id, r, "");
    else if (type == "GUIProgressBar")
        m_guiSystem->AddProgressBar(sn, id, r, 0.5f);
}

// ── Selection ──────────────────────────────────────────────────

void GUIEditor::SelectElement(UI::GUIElement* elem) {
    m_state.selectedElement = elem;
}

void GUIEditor::DeselectAll() {
    m_state.selectedElement = nullptr;
}

void GUIEditor::DeleteSelected() {
    if (!m_guiSystem || !m_state.selectedElement) return;
    auto* screen = m_guiSystem->GetScreen(m_state.activeScreenName);
    if (!screen) return;
    auto& elems = screen->elements;
    elems.erase(std::remove_if(elems.begin(), elems.end(),
        [this](const std::shared_ptr<UI::GUIElement>& e){
            return e.get() == m_state.selectedElement;
        }), elems.end());
    m_state.selectedElement = nullptr;
}

void GUIEditor::DuplicateSelected() {
    // Shallow clone not supported without RTTI — mark as TODO
    // Future: serialise + deserialise the element
}

// ── Transform ──────────────────────────────────────────────────

void GUIEditor::MoveSelected(float dx, float dy) {
    if (!m_state.selectedElement) return;
    m_state.selectedElement->rect.x += dx;
    m_state.selectedElement->rect.y += dy;
    if (m_state.snapToGrid) SnapToGrid(m_state.selectedElement->rect);
    if (m_onPropertyChanged) m_onPropertyChanged(m_state.selectedElement);
}

void GUIEditor::ResizeSelected(float dw, float dh) {
    if (!m_state.selectedElement) return;
    m_state.selectedElement->rect.w = std::max(4.f, m_state.selectedElement->rect.w + dw);
    m_state.selectedElement->rect.h = std::max(4.f, m_state.selectedElement->rect.h + dh);
    if (m_state.snapToGrid) SnapToGrid(m_state.selectedElement->rect);
    if (m_onPropertyChanged) m_onPropertyChanged(m_state.selectedElement);
}

// ── Callbacks ──────────────────────────────────────────────────

void GUIEditor::OnPropertyChanged(PropertyChangedCallback cb) {
    m_onPropertyChanged = std::move(cb);
}

// ── Undo / Redo ────────────────────────────────────────────────

void GUIEditor::Undo() {
    if (m_historyPos < 0) return;
    m_history[m_historyPos--].undo();
}

void GUIEditor::Redo() {
    if (m_historyPos + 1 >= static_cast<int>(m_history.size())) return;
    m_history[++m_historyPos].execute();
}

// ── Mode ───────────────────────────────────────────────────────

void GUIEditor::SetMode(GUIEditorMode mode) {
    m_state.mode = mode;
}

// ── Private render stubs ───────────────────────────────────────

void GUIEditor::RenderToolbar()    { /* Mode buttons, add-element palette */ }
void GUIEditor::RenderScreenList() { /* List of screens in the layout file */ }
void GUIEditor::RenderCanvas()     { /* Draw elements on a 2-D canvas with zoom/pan */ }
void GUIEditor::RenderInspector()  { /* Property fields for the selected element */ }

// ── Snap helper ────────────────────────────────────────────────

void GUIEditor::SnapToGrid(UI::GUIRect& rect) const {
    float g = m_state.gridSize;
    if (g <= 0.f) return;
    rect.x = std::round(rect.x / g) * g;
    rect.y = std::round(rect.y / g) * g;
    rect.w = std::max(g, std::round(rect.w / g) * g);
    rect.h = std::max(g, std::round(rect.h / g) * g);
}

} // namespace Editor
