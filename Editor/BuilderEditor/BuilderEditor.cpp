#include "Editor/BuilderEditor/BuilderEditor.h"
#include <cstring>

namespace Editor {

void BuilderEditor::SetMode(BuilderEditorMode mode) { m_mode = mode; }
BuilderEditorMode BuilderEditor::GetMode() const { return m_mode; }

void BuilderEditor::BeginPlacement(uint32_t partDefId) {
    m_mode = BuilderEditorMode::PlacePart;
    m_preview = PlacementPreview{};
    m_preview.partDefId = partDefId;
    m_preview.isValid = (m_partLibrary && m_partLibrary->Get(partDefId) != nullptr);
}

void BuilderEditor::UpdatePlacementTransform(const float transform[16]) {
    std::memcpy(m_preview.transform, transform, sizeof(float) * 16);
    m_preview.isValid = (m_partLibrary && m_partLibrary->Get(m_preview.partDefId) != nullptr);
}

void BuilderEditor::CommitPlacement() {
    if (!m_preview.isValid || !m_assembly || !m_partLibrary) {
        m_lastError = "Cannot commit: invalid preview or missing assembly/library";
        return;
    }
    uint32_t instId = m_assembly->AddPart(m_preview.partDefId, m_preview.transform);
    if (m_preview.snapTargetPartId != 0 && m_preview.snapTargetPointId != 0 && m_snapRules) {
        m_assembly->Link(instId, 0, m_preview.snapTargetPartId, m_preview.snapTargetPointId, false);
    }
    m_preview = PlacementPreview{};
    m_mode = BuilderEditorMode::None;
}

void BuilderEditor::CancelPlacement() {
    m_preview = PlacementPreview{};
    m_mode = BuilderEditorMode::None;
}

void BuilderEditor::SelectPart(uint32_t instanceId) {
    m_selectedInstanceId = instanceId;
    m_mode = BuilderEditorMode::SelectMode;
}

void BuilderEditor::DeselectPart() {
    m_selectedInstanceId = 0;
}

void BuilderEditor::DeleteSelectedPart() {
    if (m_selectedInstanceId == 0 || !m_assembly) return;
    m_assembly->RemovePart(m_selectedInstanceId);
    m_selectedInstanceId = 0;
}

void BuilderEditor::WeldSelected(uint32_t toInstanceId, uint32_t toSnapId) {
    if (m_selectedInstanceId == 0 || !m_assembly) return;
    m_assembly->Link(m_selectedInstanceId, 0, toInstanceId, toSnapId, true);
}

void BuilderEditor::UnweldSelected() {
    if (m_selectedInstanceId == 0 || !m_assembly) return;
    m_assembly->Unlink(m_selectedInstanceId, 0);
}

void BuilderEditor::SetAssembly(Builder::Assembly* assembly) { m_assembly = assembly; }
Builder::Assembly* BuilderEditor::GetAssembly() const { return m_assembly; }

void BuilderEditor::SetPartLibrary(Builder::PartLibrary* library) { m_partLibrary = library; }
Builder::PartLibrary* BuilderEditor::GetPartLibrary() const { return m_partLibrary; }

void BuilderEditor::SetSnapRules(Builder::SnapRules* snapRules) { m_snapRules = snapRules; }
Builder::SnapRules* BuilderEditor::GetSnapRules() const { return m_snapRules; }

const PlacementPreview& BuilderEditor::GetPlacementPreview() const { return m_preview; }
uint32_t BuilderEditor::GetSelectedInstanceId() const { return m_selectedInstanceId; }

bool BuilderEditor::Validate() {
    if (!m_assembly) { m_lastError = "No assembly set"; return false; }
    bool ok = m_assembly->Validate();
    if (!ok) m_lastError = "Assembly validation failed: duplicate snap link detected";
    else m_lastError.clear();
    return ok;
}

const std::string& BuilderEditor::GetLastError() const { return m_lastError; }

} // namespace Editor
