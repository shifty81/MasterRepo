#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace Core {

// --- Compile-time string hashing for event type IDs ---

using EventTypeID = uint64_t;

constexpr EventTypeID HashEventType(std::string_view name) noexcept {
    // FNV-1a 64-bit hash
    uint64_t hash = 14695981039346656037ULL;
    for (char c : name) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

// --- Event categories (bitfield) ---

enum class EventCategory : uint32_t {
    None        = 0,
    Application = 1 << 0,
    Input       = 1 << 1,
    Keyboard    = 1 << 2,
    Mouse       = 1 << 3,
    Editor      = 1 << 4,
    Engine      = 1 << 5
};

inline constexpr EventCategory operator|(EventCategory lhs, EventCategory rhs) noexcept {
    return static_cast<EventCategory>(
        static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline constexpr EventCategory operator&(EventCategory lhs, EventCategory rhs) noexcept {
    return static_cast<EventCategory>(
        static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}

inline constexpr bool HasCategory(EventCategory field, EventCategory category) noexcept {
    return (field & category) != EventCategory::None;
}

// --- Macro to declare event type boilerplate in derived classes ---
//
// Usage:
//   class WindowCloseEvent : public Core::Event {
//   public:
//       EVENT_CLASS_TYPE("WindowCloseEvent")
//       EventCategory GetCategoryFlags() const override {
//           return EventCategory::Application;
//       }
//   };

#define EVENT_CLASS_TYPE(typeName)                                           \
    static constexpr Core::EventTypeID StaticType() noexcept {              \
        return Core::HashEventType(typeName);                               \
    }                                                                       \
    Core::EventTypeID GetTypeID() const noexcept override {                 \
        return StaticType();                                                \
    }                                                                       \
    std::string_view GetName() const noexcept override {                    \
        return typeName;                                                    \
    }

// --- Base Event class ---

class Event {
public:
    virtual ~Event() = default;

    virtual EventTypeID   GetTypeID()        const noexcept = 0;
    virtual std::string_view GetName()       const noexcept = 0;
    virtual EventCategory GetCategoryFlags() const = 0;

    bool IsInCategory(EventCategory category) const noexcept {
        return HasCategory(GetCategoryFlags(), category);
    }

    bool Handled = false;

    /// Timestamp in seconds since engine start.
    /// Must be set manually by the caller before publishing; EventBus does
    /// not populate this automatically to avoid a clock dependency.
    double Timestamp = 0.0;
};

// --- EventDispatcher ---
//
// Dispatches a single event to a handler if the event matches the expected
// type. Typical usage inside an OnEvent(Event&) callback:
//
//   EventDispatcher dispatcher(event);
//   dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) {
//       // handle it
//       return true;
//   });

class EventDispatcher {
public:
    explicit EventDispatcher(Event& event) noexcept
        : m_Event(event) {}

    /// Calls `handler` if the runtime event type matches T::StaticType().
    /// Returns true if the handler was invoked.
    template <typename T, typename F>
    bool Dispatch(F&& handler) {
        if (m_Event.GetTypeID() == T::StaticType()) {
            m_Event.Handled |= handler(static_cast<T&>(m_Event));
            return true;
        }
        return false;
    }

private:
    Event& m_Event;
};

} // namespace Core
