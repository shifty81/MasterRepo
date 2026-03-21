#pragma once
/**
 * @file Signal.h
 * @brief Lightweight typed Signal<Args...> with connect/disconnect/emit.
 *
 * API:
 *   Signal<int, std::string> sig;
 *   auto id = sig.Connect([](int x, const std::string& s){ ... });
 *   sig.Emit(42, "hello");
 *   sig.Disconnect(id);
 *
 *   // RAII lifetime via ScopedConnection
 *   ScopedConnection sc = sig.ConnectScoped([](int x, const std::string& s){ ... });
 *   // Disconnects automatically when sc goes out of scope.
 *
 * Design:
 *   - Not thread-safe by default (single-threaded game engine use).
 *   - Handles self-disconnect-from-handler via deferred erase.
 *   - ScopedConnection moves ownership; not copyable.
 */

#include <cstdint>
#include <functional>
#include <vector>
#include <algorithm>

namespace Core {

using ConnectionID = uint64_t;
constexpr ConnectionID kInvalidConnection = 0;

// ── ScopedConnection ──────────────────────────────────────────────────────
// Forward declare
template<typename... Args> class Signal;

class ScopedConnection {
public:
    ScopedConnection() = default;
    ScopedConnection(ScopedConnection&&) noexcept;
    ScopedConnection& operator=(ScopedConnection&&) noexcept;
    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;
    ~ScopedConnection();

    bool Valid() const { return m_disconnect != nullptr; }
    void Disconnect();

private:
    template<typename... Args> friend class Signal;
    std::function<void()> m_disconnect;

    explicit ScopedConnection(std::function<void()> disconnectFn)
        : m_disconnect(std::move(disconnectFn)) {}
};

inline ScopedConnection::ScopedConnection(ScopedConnection&& o) noexcept
    : m_disconnect(std::move(o.m_disconnect))
{ o.m_disconnect = nullptr; }

inline ScopedConnection& ScopedConnection::operator=(ScopedConnection&& o) noexcept {
    if (this != &o) {
        Disconnect();
        m_disconnect = std::move(o.m_disconnect);
        o.m_disconnect = nullptr;
    }
    return *this;
}
inline ScopedConnection::~ScopedConnection() { Disconnect(); }
inline void ScopedConnection::Disconnect() {
    if (m_disconnect) { m_disconnect(); m_disconnect = nullptr; }
}

// ── Signal ────────────────────────────────────────────────────────────────
template<typename... Args>
class Signal {
public:
    using SlotFn = std::function<void(Args...)>;

    Signal() = default;
    ~Signal() = default;
    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

    /// Connect a slot; returns an ID that can be passed to Disconnect().
    ConnectionID Connect(SlotFn fn) {
        m_slots.push_back({m_nextId, false, std::move(fn)});
        return m_nextId++;
    }

    /// Connect and return a ScopedConnection (auto-disconnects on destruction).
    ScopedConnection ConnectScoped(SlotFn fn) {
        ConnectionID id = Connect(std::move(fn));
        return ScopedConnection([this, id](){ Disconnect(id); });
    }

    void Disconnect(ConnectionID id) {
        if (m_emitting) {
            // Deferred: mark as tombstone
            for (auto& s : m_slots) if (s.id == id) { s.dead = true; return; }
        } else {
            m_slots.erase(
                std::remove_if(m_slots.begin(), m_slots.end(),
                    [id](const Slot& s){ return s.id == id; }),
                m_slots.end());
        }
    }

    void DisconnectAll() {
        if (m_emitting) {
            for (auto& s : m_slots) s.dead = true;
        } else {
            m_slots.clear();
        }
    }

    bool HasConnections() const {
        for (const auto& s : m_slots) if (!s.dead) return true;
        return false;
    }
    size_t ConnectionCount() const {
        size_t n = 0;
        for (const auto& s : m_slots) if (!s.dead) ++n;
        return n;
    }

    void Emit(Args... args) {
        m_emitting = true;
        // Iterate by index to allow new connects during emit
        size_t n = m_slots.size();
        for (size_t i = 0; i < n; ++i) {
            if (!m_slots[i].dead && m_slots[i].fn)
                m_slots[i].fn(args...);
        }
        m_emitting = false;
        // Purge tombstones
        m_slots.erase(
            std::remove_if(m_slots.begin(), m_slots.end(),
                [](const Slot& s){ return s.dead; }),
            m_slots.end());
    }

    // Allow signals to be called like functions
    void operator()(Args... args) { Emit(std::forward<Args>(args)...); }

private:
    struct Slot {
        ConnectionID id{0};
        bool         dead{false};
        SlotFn       fn;
    };

    std::vector<Slot> m_slots;
    ConnectionID      m_nextId{1};
    bool              m_emitting{false};
};

} // namespace Core
