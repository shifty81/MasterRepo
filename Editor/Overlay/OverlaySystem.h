#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace Editor {

enum class NotificationLevel { Info, Warning, Error, Success };

struct Notification {
    uint32_t          id          = 0;
    std::string       message;
    NotificationLevel level       = NotificationLevel::Info;
    uint64_t          timestamp   = 0;
    uint32_t          duration_ms = 3000;
    bool              dismissed   = false;
};

struct DebugOverlay {
    uint32_t    id       = 0;
    std::string category;
    std::string key;
    std::string value;
    uint32_t    color   = 0xFFFFFFFF;
    bool        visible = true;
};

struct ToolOverlay {
    uint32_t    id       = 0;
    std::string toolName;
    std::string helpText;
    std::string shortcut;
    bool        visible = true;
};

class OverlayPanel {
public:
    uint32_t PostNotification(const std::string& msg, NotificationLevel level, uint32_t duration_ms = 3000);
    void     DismissNotification(uint32_t id);
    void     Tick(uint32_t delta_ms);

    uint32_t AddDebugEntry(const std::string& category, const std::string& key,
                           const std::string& value, uint32_t color = 0xFFFFFFFF);
    void     UpdateDebugEntry(uint32_t id, const std::string& value);
    void     RemoveDebugEntry(uint32_t id);

    uint32_t RegisterToolOverlay(const std::string& toolName, const std::string& helpText,
                                  const std::string& shortcut);
    void     SetToolOverlayVisible(uint32_t id, bool visible);

    const std::vector<Notification>&  GetNotifications()  const { return m_notifications; }
    const std::vector<DebugOverlay>&  GetDebugEntries()   const { return m_debugEntries; }
    const std::vector<ToolOverlay>&   GetToolOverlays()   const { return m_toolOverlays; }

    void ClearAll();

private:
    uint32_t m_nextId = 1;
    uint64_t m_elapsed = 0;
    std::vector<Notification>  m_notifications;
    std::vector<DebugOverlay>  m_debugEntries;
    std::vector<ToolOverlay>   m_toolOverlays;
};

} // namespace Editor
