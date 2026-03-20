#pragma once
#include "Builder/Assembly/Assembly.h"
#include "Builder/Parts/PartDef.h"
#include "Builder/SnapRules/SnapRules.h"
#include <cstdint>
#include <string>

namespace Editor {

enum class BuilderEditorMode { None, PlacePart, SnapAlign, WeldMode, SelectMode };

struct PlacementPreview {
    uint32_t partDefId         = 0;
    float    transform[16]     = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    bool     isValid           = false;
    uint32_t snapTargetPartId  = 0;
    uint32_t snapTargetPointId = 0;
};

class BuilderEditor {
public:
    void SetMode(BuilderEditorMode mode);
    BuilderEditorMode GetMode() const;

    void BeginPlacement(uint32_t partDefId);
    void UpdatePlacementTransform(const float transform[16]);
    void CommitPlacement();
    void CancelPlacement();

    void SelectPart(uint32_t instanceId);
    void DeselectPart();
    void DeleteSelectedPart();

    void WeldSelected(uint32_t toInstanceId, uint32_t toSnapId);
    void UnweldSelected();

    void SetAssembly(Builder::Assembly* assembly);
    Builder::Assembly* GetAssembly() const;

    void SetPartLibrary(Builder::PartLibrary* library);
    Builder::PartLibrary* GetPartLibrary() const;

    void SetSnapRules(Builder::SnapRules* snapRules);
    Builder::SnapRules* GetSnapRules() const;

    const PlacementPreview& GetPlacementPreview() const;
    uint32_t GetSelectedInstanceId() const;

    bool Validate();
    const std::string& GetLastError() const;

private:
    BuilderEditorMode  m_mode             = BuilderEditorMode::None;
    PlacementPreview   m_preview;
    uint32_t           m_selectedInstanceId = 0;
    Builder::Assembly* m_assembly          = nullptr;
    Builder::PartLibrary* m_partLibrary    = nullptr;
    Builder::SnapRules*   m_snapRules      = nullptr;
    std::string        m_lastError;
};

} // namespace Editor
