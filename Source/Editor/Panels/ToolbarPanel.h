#pragma once

#include <string>
#include <vector>

namespace nf
{
    class EditorCommandRegistry;
    struct EditorToolContext;

    struct ToolbarButton
    {
        std::string label;
        std::string commandId;
        bool isToggle = false;
    };

    struct ToolbarGroup
    {
        std::string label;
        std::vector<ToolbarButton> buttons;
    };

    class ToolbarPanel
    {
    public:
        explicit ToolbarPanel(EditorCommandRegistry* commandRegistry = nullptr);

        void SetCommandRegistry(EditorCommandRegistry* commandRegistry);
        void BuildDefaultButtons();

        const std::vector<ToolbarGroup>& GetGroups() const noexcept;
        bool Trigger(const std::string& commandId) const;

        static bool IsToolCommand(const std::string& commandId);

    private:
        EditorCommandRegistry* m_commandRegistry = nullptr;
        std::vector<ToolbarGroup> m_groups;
    };
}
