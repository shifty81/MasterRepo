#pragma once
#include "ITool.h"
#include <memory>
#include <string>
#include <vector>

namespace atlas::tools {

/// Manages the lifecycle of all registered in-client editor tools.
///
/// The registry owns ITool instances, handles activation/deactivation
/// (only one tool active at a time by default), and drives per-frame
/// Update() on the active tool.  Compiled out when ATLAS_EDITOR_TOOLS
/// is not defined.
class EditorToolRegistry {
public:
    /// Register a tool.  The registry takes ownership.
    void Register(std::unique_ptr<ITool> tool);

    /// Number of registered tools.
    size_t Count() const;

    /// Get tool by index (nullptr if out of range).
    ITool* Get(size_t index) const;

    /// Find tool by name (nullptr if not found).
    ITool* FindByName(const std::string& name) const;

    /// Activate a tool by name.  Deactivates the currently active tool
    /// first (if any).  Returns false if the tool wasn't found.
    bool Activate(const std::string& name);

    /// Deactivate the currently active tool.
    void DeactivateCurrent();

    /// Get the currently active tool (nullptr if none).
    ITool* ActiveTool() const;

    /// Update the active tool (call once per frame).
    void UpdateActive(float deltaTime);

    /// All registered tool names (in registration order).
    std::vector<std::string> ToolNames() const;

    /// Remove all registered tools.
    void Clear();

private:
    std::vector<std::unique_ptr<ITool>> m_tools;
    ITool* m_active = nullptr;
};

} // namespace atlas::tools
