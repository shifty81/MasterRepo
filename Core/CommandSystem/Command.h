#pragma once

#include <memory>
#include <string>

namespace Core::CommandSystem {

/// Abstract interface for an undoable command (Command pattern).
///
/// Derive from ICommand and implement Execute(), Undo(), and GetName()
/// to create operations that can be pushed through the CommandSystem for
/// full undo/redo support.
class ICommand {
public:
    virtual ~ICommand() = default;

    /// Perform the command's action.
    virtual void Execute() = 0;

    /// Reverse the command's action.
    virtual void Undo() = 0;

    /// Human-readable description of this command (for UI / logging).
    [[nodiscard]] virtual std::string GetName() const = 0;
};

/// Convenience alias.
using CommandPtr = std::unique_ptr<ICommand>;

} // namespace Core::CommandSystem
