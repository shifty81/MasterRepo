#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace atlas::tools {

/// Identifies a command type in the editor command bus.
enum class EditorCommandType : uint8_t {
    SelectEntity,
    TransformEntity,
    DeleteEntity,
    DuplicateEntity,
    CreateEntity,
    SetProperty,
    Undo,
    Redo,
    ToggleTool,
    SaveBookmark,
    RestoreBookmark,
    SetLayerVisibility,
    Custom
};

/// A single editor command flowing through the bus.
struct EditorCommand {
    EditorCommandType type = EditorCommandType::Custom;
    uint32_t entityId = 0;
    std::string key;
    std::string value;
    float floatValue = 0.0f;
};

/// FIFO command queue with handler dispatch for editor commands.
///
/// Thread-safe.  Commands are enqueued from any thread and dispatched
/// on the editor thread via Dispatch().
class EditorCommandBus {
public:
    void Enqueue(const EditorCommand& cmd);
    void Enqueue(EditorCommand&& cmd);

    /// Move all pending commands into `out` (clears the internal queue).
    void Drain(std::vector<EditorCommand>& out);

    size_t PendingCount() const;
    void Clear();

    using Handler = std::function<void(const EditorCommand&)>;

    /// Register a handler for a specific command type.
    void RegisterHandler(EditorCommandType type, Handler handler);

    /// Dispatch all pending commands to registered handlers, then clear.
    void Dispatch();

private:
    mutable std::mutex m_mutex;
    std::vector<EditorCommand> m_pending;
    std::map<uint8_t, std::vector<Handler>> m_handlers;
};

} // namespace atlas::tools
