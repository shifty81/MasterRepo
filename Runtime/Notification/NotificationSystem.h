#pragma once
/**
 * @file NotificationSystem.h
 * @brief In-game notification / toast message system with priority queue,
 *        display duration, and category filtering.
 *
 * Notifications are queued, dequeued, and expired automatically each tick.
 * Categories (e.g. "combat", "mission", "achievement") can be enabled or
 * muted individually.
 *
 * Typical usage:
 * @code
 *   NotificationSystem notif;
 *   notif.Init();
 *   notif.Post("Mission Complete!", "You destroyed the enemy flagship.",
 *              NotificationCategory::Mission, NotificationPriority::High);
 *   notif.Tick(deltaSeconds);
 *   auto active = notif.ActiveNotifications();
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

// ── Notification category ─────────────────────────────────────────────────────

enum class NotificationCategory : uint8_t {
    System      = 0,
    Mission     = 1,
    Combat      = 2,
    Achievement = 3,
    Economy     = 4,
    Social      = 5,
    Debug       = 6,
    COUNT
};

// ── Priority ──────────────────────────────────────────────────────────────────

enum class NotificationPriority : uint8_t { Low = 0, Normal = 1, High = 2, Critical = 3 };

// ── A single notification ─────────────────────────────────────────────────────

struct Notification {
    uint32_t              id{0};
    std::string           title;
    std::string           body;
    NotificationCategory  category{NotificationCategory::System};
    NotificationPriority  priority{NotificationPriority::Normal};
    float                 displayDuration{4.f}; ///< seconds; 0 = persistent
    float                 remainingSeconds{0.f};
    uint64_t              timestampMs{0};
    std::string           iconName;    ///< optional icon identifier
    std::string           actionLabel; ///< optional button label
    std::function<void()> actionFn;    ///< optional button callback
};

// ── NotificationSystem ────────────────────────────────────────────────────────

class NotificationSystem {
public:
    NotificationSystem();
    ~NotificationSystem();

    void Init();
    void Shutdown();

    // ── Posting ───────────────────────────────────────────────────────────────

    uint32_t Post(const std::string& title, const std::string& body = "",
                  NotificationCategory category = NotificationCategory::System,
                  NotificationPriority priority = NotificationPriority::Normal,
                  float displayDuration = 4.f);

    uint32_t PostWithAction(const std::string& title, const std::string& body,
                            const std::string& actionLabel,
                            std::function<void()> actionFn,
                            NotificationCategory category = NotificationCategory::System,
                            NotificationPriority priority = NotificationPriority::Normal);

    void     Dismiss(uint32_t notificationId);
    void     DismissAll();
    void     DismissCategory(NotificationCategory category);

    // ── Queries ───────────────────────────────────────────────────────────────

    std::vector<Notification> ActiveNotifications()  const;
    std::vector<Notification> HistoryLog()           const;
    uint32_t                  ActiveCount()          const;

    // ── Category muting ───────────────────────────────────────────────────────

    void MuteCategory(NotificationCategory category);
    void UnmuteCategory(NotificationCategory category);
    bool IsCategoryMuted(NotificationCategory category) const;

    // ── Capacity ──────────────────────────────────────────────────────────────

    void SetMaxActive(uint32_t max);     ///< oldest dismissed when limit reached
    void SetHistoryLimit(uint32_t max);

    // ── Tick ──────────────────────────────────────────────────────────────────

    void Tick(float deltaSeconds);

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnNotificationPosted(std::function<void(const Notification&)> cb);
    void OnNotificationExpired(std::function<void(const Notification&)> cb);
    void OnNotificationDismissed(std::function<void(uint32_t id)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Runtime
