#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace nf
{
    class EditorApp;
    class ConsolePanel;
    class ContentBrowser;
    class Inspector;

    struct EditorCommandContext
    {
        EditorApp* app = nullptr;
        ConsolePanel* console = nullptr;
        ContentBrowser* contentBrowser = nullptr;
        Inspector* inspector = nullptr;
    };

    struct EditorCommand
    {
        std::string id;
        std::string displayName;
        std::function<bool(const EditorCommandContext&)> canExecute;
        std::function<void(EditorCommandContext&)> execute;
    };

    class EditorCommandRegistry
    {
    public:
        explicit EditorCommandRegistry(EditorCommandContext context = {});

        void SetContext(EditorCommandContext context);
        const EditorCommandContext& GetContext() const noexcept;

        bool Register(EditorCommand command);
        bool CanExecute(const std::string& id) const;
        bool Execute(const std::string& id);

        const EditorCommand* Find(const std::string& id) const;
        std::vector<std::string> GetRegisteredCommandIds() const;

    private:
        EditorCommandContext m_context;
        std::unordered_map<std::string, EditorCommand> m_commands;
    };
}
