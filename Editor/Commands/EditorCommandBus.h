#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace Editor {

// ──────────────────────────────────────────────────────────────
// EditorCommandBus — migrated from atlas::tools::EditorCommandBus
// ──────────────────────────────────────────────────────────────

enum class EditorCommandType : uint8_t {
    SelectEntity, TransformEntity, DeleteEntity, DuplicateEntity, CreateEntity,
    SetProperty, Undo, Redo, ToggleTool, SaveBookmark, RestoreBookmark,
    SetLayerVisibility, Custom
};

struct EditorCommand {
    EditorCommandType type       = EditorCommandType::Custom;
    uint32_t          entityId   = 0;
    std::string       key;
    std::string       value;
    float             floatValue = 0.0f;
};

class EditorCommandBus {
public:
    void Enqueue(const EditorCommand& cmd);
    void Enqueue(EditorCommand&& cmd);
    void Drain(std::vector<EditorCommand>& out);
    size_t PendingCount() const;
    void Clear();

    using Handler = std::function<void(const EditorCommand&)>;
    void RegisterHandler(EditorCommandType type, Handler handler);
    void Dispatch();

private:
    mutable std::mutex           m_mutex;
    std::vector<EditorCommand>   m_pending;
    std::map<uint8_t, std::vector<Handler>> m_handlers;
};

} // namespace Editor
