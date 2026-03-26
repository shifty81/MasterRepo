#include "Runtime/Notification/NotificationSystem.h"
#include <algorithm>
#include <chrono>
#include <list>
#include <vector>

namespace Runtime {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

struct NotificationSystem::Impl {
    std::list<Notification>   active;
    std::vector<Notification> history;
    uint32_t                  nextId{1};
    uint32_t                  maxActive{5};
    uint32_t                  historyLimit{200};
    bool muteTable[static_cast<int>(NotificationCategory::COUNT)]{};

    std::function<void(const Notification&)> onPosted;
    std::function<void(const Notification&)> onExpired;
    std::function<void(uint32_t)>            onDismissed;
};

NotificationSystem::NotificationSystem() : m_impl(new Impl()) {}
NotificationSystem::~NotificationSystem() { delete m_impl; }

void NotificationSystem::Init()     {}
void NotificationSystem::Shutdown() { DismissAll(); }

uint32_t NotificationSystem::Post(const std::string& title,
                                  const std::string& body,
                                  NotificationCategory cat,
                                  NotificationPriority prio,
                                  float duration) {
    int catIdx = static_cast<int>(cat);
    if (m_impl->muteTable[catIdx]) return 0;

    Notification n;
    n.id              = m_impl->nextId++;
    n.title           = title;
    n.body            = body;
    n.category        = cat;
    n.priority        = prio;
    n.displayDuration = duration;
    n.remainingSeconds = duration;
    n.timestampMs     = NowMs();

    // If at capacity, remove lowest-priority oldest
    if (m_impl->active.size() >= m_impl->maxActive) {
        auto worst = m_impl->active.begin();
        for (auto it = m_impl->active.begin(); it != m_impl->active.end(); ++it)
            if (static_cast<int>(it->priority) < static_cast<int>(worst->priority))
                worst = it;
        m_impl->active.erase(worst);
    }

    // Insert by priority (highest first)
    auto pos = m_impl->active.begin();
    while (pos != m_impl->active.end() &&
           static_cast<int>(pos->priority) >= static_cast<int>(prio))
        ++pos;
    m_impl->active.insert(pos, n);

    if (m_impl->onPosted) m_impl->onPosted(n);
    return n.id;
}

uint32_t NotificationSystem::PostWithAction(const std::string& title,
    const std::string& body, const std::string& label,
    std::function<void()> actionFn, NotificationCategory cat,
    NotificationPriority prio) {
    uint32_t id = Post(title, body, cat, prio);
    for (auto& n : m_impl->active) {
        if (n.id == id) {
            n.actionLabel = label;
            n.actionFn    = std::move(actionFn);
            break;
        }
    }
    return id;
}

void NotificationSystem::Dismiss(uint32_t id) {
    for (auto it = m_impl->active.begin(); it != m_impl->active.end(); ++it) {
        if (it->id == id) {
            if (m_impl->history.size() < m_impl->historyLimit)
                m_impl->history.push_back(*it);
            m_impl->active.erase(it);
            if (m_impl->onDismissed) m_impl->onDismissed(id);
            return;
        }
    }
}

void NotificationSystem::DismissAll() {
    while (!m_impl->active.empty()) Dismiss(m_impl->active.front().id);
}

void NotificationSystem::DismissCategory(NotificationCategory cat) {
    std::vector<uint32_t> toRemove;
    for (auto& n : m_impl->active) if (n.category == cat) toRemove.push_back(n.id);
    for (auto id : toRemove) Dismiss(id);
}

std::vector<Notification> NotificationSystem::ActiveNotifications() const {
    return {m_impl->active.begin(), m_impl->active.end()};
}

std::vector<Notification> NotificationSystem::HistoryLog() const {
    return m_impl->history;
}

uint32_t NotificationSystem::ActiveCount() const {
    return static_cast<uint32_t>(m_impl->active.size());
}

void NotificationSystem::MuteCategory(NotificationCategory c) {
    m_impl->muteTable[static_cast<int>(c)] = true;
}

void NotificationSystem::UnmuteCategory(NotificationCategory c) {
    m_impl->muteTable[static_cast<int>(c)] = false;
}

bool NotificationSystem::IsCategoryMuted(NotificationCategory c) const {
    return m_impl->muteTable[static_cast<int>(c)];
}

void NotificationSystem::SetMaxActive(uint32_t max)   { m_impl->maxActive    = max; }
void NotificationSystem::SetHistoryLimit(uint32_t max){ m_impl->historyLimit = max; }

void NotificationSystem::Tick(float dt) {
    std::vector<uint32_t> expired;
    for (auto& n : m_impl->active) {
        if (n.displayDuration <= 0.f) continue;  // persistent
        n.remainingSeconds -= dt;
        if (n.remainingSeconds <= 0.f) {
            if (m_impl->onExpired) m_impl->onExpired(n);
            expired.push_back(n.id);
        }
    }
    for (auto id : expired) Dismiss(id);
}

void NotificationSystem::OnNotificationPosted(std::function<void(const Notification&)> cb) {
    m_impl->onPosted = std::move(cb);
}

void NotificationSystem::OnNotificationExpired(std::function<void(const Notification&)> cb) {
    m_impl->onExpired = std::move(cb);
}

void NotificationSystem::OnNotificationDismissed(std::function<void(uint32_t)> cb) {
    m_impl->onDismissed = std::move(cb);
}

} // namespace Runtime
