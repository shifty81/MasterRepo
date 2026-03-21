#pragma once
/**
 * @file InputMapper.h
 * @brief Action-map layer over raw input: named actions, analog/digital, rebind, deadzone.
 *
 * InputMapper sits between raw device input and game logic:
 *   - ActionDef: named action with a list of Binding (key/button/axis) and
 *     optional deadzone, invert, scale for analog axes.
 *   - InputMapper::Update(rawInput) processes all bindings and fires callbacks.
 *   - Actions can be queried as digital (IsPressed/IsJustPressed/IsJustReleased)
 *     or as analog float [-1, 1].
 *   - Bindings are rebindable at runtime; serialise/load from JSON-compatible struct.
 *   - Supports composite "virtual axes" (e.g. left=A, right=D → float [-1,1]).
 */

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>

namespace Runtime::Input {

// ── Raw device codes ──────────────────────────────────────────────────────
enum class DeviceType : uint8_t { Keyboard, Mouse, Gamepad };
using InputCode = uint32_t;

// Well-known keyboard codes (ASCII-compatible for printable keys)
namespace Key {
    constexpr InputCode A=65,B=66,C=67,D=68,E=69,F=70,G=71,H=72,I=73,J=74,
        K=75,L=76,M=77,N=78,O=79,P=80,Q=81,R=82,S=83,T=84,U=85,V=86,W=87,
        X=88,Y=89,Z=90;
    constexpr InputCode Up=256,Down=257,Left=258,Right=259;
    constexpr InputCode Space=32,Enter=257+4,Escape=257+5,Shift=257+6,Ctrl=257+7;
}

// ── Binding ───────────────────────────────────────────────────────────────
enum class BindingType : uint8_t {
    Digital,        ///< Single key/button → 0 or 1
    AnalogAxis,     ///< Raw axis (e.g. left stick X) → [-1,1]
    CompositeAxis,  ///< Two keys mapped to -1 and +1 respectively
};

struct Binding {
    BindingType  type{BindingType::Digital};
    DeviceType   device{DeviceType::Keyboard};
    InputCode    code{0};          ///< Primary code (key, button, or axis index)
    InputCode    negCode{0};       ///< Used only for CompositeAxis (negative direction)
    float        deadzone{0.1f};
    float        scale{1.0f};
    bool         invert{false};
};

// ── Action definition ─────────────────────────────────────────────────────
struct ActionDef {
    std::string           name;
    std::vector<Binding>  bindings;
    bool                  isAnalog{false};
};

// ── Raw input frame (caller fills this each frame) ────────────────────────
struct RawInputFrame {
    std::vector<InputCode>  keysHeld;
    std::vector<InputCode>  keysPressed;   ///< Newly pressed this frame
    std::vector<InputCode>  keysReleased;
    std::unordered_map<InputCode, float> axes;  ///< Gamepad/mouse axes
};

// ── Action state ──────────────────────────────────────────────────────────
struct ActionState {
    float value{0.0f};        ///< Analog value or 0/1 for digital
    bool  held{false};
    bool  justPressed{false};
    bool  justReleased{false};
};

// ── Callbacks ─────────────────────────────────────────────────────────────
using ActionCallback = std::function<void(const std::string& action, const ActionState&)>;

// ── Mapper ────────────────────────────────────────────────────────────────
class InputMapper {
public:
    InputMapper();
    ~InputMapper();

    // ── define actions ────────────────────────────────────────
    void RegisterAction(const ActionDef& def);
    void RemoveAction(const std::string& name);
    bool HasAction(const std::string& name) const;
    const ActionDef* GetAction(const std::string& name) const;
    std::vector<std::string> ActionNames() const;

    // ── rebinding ─────────────────────────────────────────────
    /// Replace all bindings for a registered action.
    bool RebindAction(const std::string& name, std::vector<Binding> bindings);
    /// Add a single binding to an existing action.
    bool AddBinding(const std::string& name, const Binding& binding);
    /// Remove binding by primary code.
    bool RemoveBinding(const std::string& name, InputCode code);

    // ── update ────────────────────────────────────────────────
    /// Call once per frame with the raw device state.
    void Update(const RawInputFrame& frame);

    // ── query ─────────────────────────────────────────────────
    bool  IsHeld(const std::string& action) const;
    bool  IsJustPressed(const std::string& action) const;
    bool  IsJustReleased(const std::string& action) const;
    float AxisValue(const std::string& action) const;
    const ActionState* GetState(const std::string& action) const;

    // ── callbacks ─────────────────────────────────────────────
    /// Fired for every action whose state changed this frame.
    void OnAction(ActionCallback cb);

    // ── serialisation ─────────────────────────────────────────
    /// Export all action definitions to a flat key=value map.
    std::unordered_map<std::string, std::string> Serialize() const;
    /// Restore from a previously serialised map.
    void Deserialize(const std::unordered_map<std::string, std::string>& data);

    // ── convenience factory ───────────────────────────────────
    /// Register a simple digital action bound to a single keyboard key.
    void MapKey(const std::string& actionName, InputCode keyCode);
    /// Register a composite horizontal axis (leftKey → -1, rightKey → +1).
    void MapAxis(const std::string& actionName, InputCode negKey, InputCode posKey);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime::Input
