#include "Editor/Panels/MenuBarPanel.h"

#include "Editor/Commands/EditorCommandRegistry.h"

namespace nf
{
    MenuBarPanel::MenuBarPanel(EditorCommandRegistry* commandRegistry)
        : m_commandRegistry(commandRegistry)
    {
    }

    void MenuBarPanel::SetCommandRegistry(EditorCommandRegistry* commandRegistry)
    {
        m_commandRegistry = commandRegistry;
    }

    void MenuBarPanel::BuildDefaultMenus()
    {
        m_menus.clear();

        m_menus.push_back(MenuGroup{
            "File",
            {
                {"New World", "File.NewWorld"},
                {"Open World", "File.OpenWorld"},
                {"Save World", "World.SaveDevWorld"},
                {"Save World As", "World.SaveDevWorldAs"},
                {"Exit", "File.Exit", true}
            }
        });

        m_menus.push_back(MenuGroup{
            "Edit",
            {
                {"Undo", "Edit.Undo"},
                {"Redo", "Edit.Redo"}
            }
        });

        m_menus.push_back(MenuGroup{
            "View",
            {
                {"Reset Layout", "View.ResetLayout"},
                {"Toggle Console", "View.ToggleConsole"},
                {"Toggle Content Browser", "View.ToggleContentBrowser"},
                {"Toggle Inspector", "View.ToggleInspector"},
                {"Toggle World Debug", "View.ToggleWorldDebug"}
            }
        });

        m_menus.push_back(MenuGroup{
            "World",
            {
                {"Load DevWorld", "World.LoadDevWorld"},
                {"Reload DevWorld", "World.ReloadDevWorld"},
                {"Save DevWorld", "World.SaveDevWorld"},
                {"Save DevWorld As", "World.SaveDevWorldAs"},
                {"Validate World State", "World.ValidateState"}
            }
        });

        m_menus.push_back(MenuGroup{
            "Tools",
            {
                {"Select", "Tools.SelectMode"},
                {"Voxel Inspect", "Tools.VoxelInspectMode"},
                {"Voxel Add", "Tools.VoxelAddMode"},
                {"Voxel Remove", "Tools.VoxelRemoveMode"}
            }
        });

        m_menus.push_back(MenuGroup{
            "Help",
            {
                {"Editor Controls", "Help.Controls"},
                {"About", "Help.About"}
            }
        });
    }

    const std::vector<MenuGroup>& MenuBarPanel::GetMenus() const noexcept
    {
        return m_menus;
    }

    bool MenuBarPanel::Trigger(const std::string& commandId) const
    {
        return m_commandRegistry != nullptr && m_commandRegistry->Execute(commandId);
    }
}
