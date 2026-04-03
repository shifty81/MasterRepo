#include "Editor/Commands/EditorCommandRegistry.h"

#include <algorithm>

namespace nf
{
    EditorCommandRegistry::EditorCommandRegistry(EditorCommandContext context)
        : m_context(context)
    {
    }

    void EditorCommandRegistry::SetContext(EditorCommandContext context)
    {
        m_context = context;
    }

    const EditorCommandContext& EditorCommandRegistry::GetContext() const noexcept
    {
        return m_context;
    }

    bool EditorCommandRegistry::Register(EditorCommand command)
    {
        if (command.id.empty() || !command.execute)
        {
            return false;
        }

        const auto [it, inserted] = m_commands.emplace(command.id, std::move(command));
        return inserted;
    }

    bool EditorCommandRegistry::CanExecute(const std::string& id) const
    {
        const EditorCommand* command = Find(id);
        if (command == nullptr)
        {
            return false;
        }

        if (!command->canExecute)
        {
            return true;
        }

        return command->canExecute(m_context);
    }

    bool EditorCommandRegistry::Execute(const std::string& id)
    {
        const EditorCommand* command = Find(id);
        if (command == nullptr)
        {
            return false;
        }

        if (command->canExecute && !command->canExecute(m_context))
        {
            return false;
        }

        command->execute(m_context);
        return true;
    }

    const EditorCommand* EditorCommandRegistry::Find(const std::string& id) const
    {
        const auto it = m_commands.find(id);
        return it != m_commands.end() ? &it->second : nullptr;
    }

    std::vector<std::string> EditorCommandRegistry::GetRegisteredCommandIds() const
    {
        std::vector<std::string> ids;
        ids.reserve(m_commands.size());

        for (const auto& [id, _] : m_commands)
        {
            ids.push_back(id);
        }

        std::sort(ids.begin(), ids.end());
        return ids;
    }
}
