#pragma once

#include <string>
#include <vector>

namespace nf
{
    class EditorCommandRegistry;

    struct MenuItem
    {
        std::string label;
        std::string commandId;
        bool separatorBefore = false;
    };

    struct MenuGroup
    {
        std::string label;
        std::vector<MenuItem> items;
    };

    class MenuBarPanel
    {
    public:
        explicit MenuBarPanel(EditorCommandRegistry* commandRegistry = nullptr);

        void SetCommandRegistry(EditorCommandRegistry* commandRegistry);
        void BuildDefaultMenus();

        const std::vector<MenuGroup>& GetMenus() const noexcept;

        bool Trigger(const std::string& commandId) const;

    private:
        EditorCommandRegistry* m_commandRegistry = nullptr;
        std::vector<MenuGroup> m_menus;
    };
}
