#include "Editor/Panels/ToolbarPanel.h"

#include "Editor/Commands/EditorCommandRegistry.h"

namespace nf
{
    ToolbarPanel::ToolbarPanel(EditorCommandRegistry* commandRegistry)
        : m_commandRegistry(commandRegistry)
    {
    }

    void ToolbarPanel::SetCommandRegistry(EditorCommandRegistry* commandRegistry)
    {
        m_commandRegistry = commandRegistry;
    }

    void ToolbarPanel::BuildDefaultButtons()
    {
        m_groups.clear();

        m_groups.push_back(ToolbarGroup{
            "File",
            {
                {"Save", "World.SaveDevWorld", false},
                {"Reload", "World.ReloadDevWorld", false}
            }
        });

        m_groups.push_back(ToolbarGroup{
            "Modes",
            {
                {"Select", "Tools.SelectMode", true},
                {"Inspect", "Tools.VoxelInspectMode", true},
                {"Add", "Tools.VoxelAddMode", true},
                {"Remove", "Tools.VoxelRemoveMode", true}
            }
        });

        m_groups.push_back(ToolbarGroup{
            "World",
            {
                {"Debug", "View.ToggleWorldDebug", true},
                {"Validate", "World.ValidateState", false}
            }
        });
    }

    const std::vector<ToolbarGroup>& ToolbarPanel::GetGroups() const noexcept
    {
        return m_groups;
    }

    bool ToolbarPanel::Trigger(const std::string& commandId) const
    {
        return m_commandRegistry != nullptr && m_commandRegistry->Execute(commandId);
    }

    bool ToolbarPanel::IsToolCommand(const std::string& commandId)
    {
        return commandId == "Tools.SelectMode"
            || commandId == "Tools.VoxelInspectMode"
            || commandId == "Tools.VoxelAddMode"
            || commandId == "Tools.VoxelRemoveMode";
    }
}
