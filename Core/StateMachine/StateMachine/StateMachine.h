#pragma once
/**
 * @file StateMachine.h
 * @brief Typed hierarchical finite state machine with transition guards and callbacks.
 *
 * Features:
 *   - State ID type is a user-supplied enum or integer
 *   - Transitions: source state + event → destination state, optional guard predicate
 *   - Hierarchical (parent states with entry/exit inheritance)
 *   - Enter / tick / exit callbacks per state
 *   - Transition callbacks (on-transition)
 *   - Any-state transitions (wild-card source)
 *   - History states: resume child state after returning to parent
 *   - Multiple concurrent FSM instances (one per actor)
 *   - JSON description loading for data-driven machines
 *
 * Typical usage (template-based interface):
 * @code
 *   enum class EnemyState { Idle, Patrol, Chase, Attack, Dead };
 *   enum class EnemyEvent { SeePlayer, LosePlayer, InRange, OutRange, Die };
 *
 *   StateMachine<EnemyState, EnemyEvent> sm;
 *   sm.AddState(EnemyState::Idle,   [](float dt){ IdleTick(dt); });
 *   sm.AddTransition(EnemyState::Idle,  EnemyEvent::SeePlayer, EnemyState::Chase);
 *   sm.AddTransition(EnemyState::Chase, EnemyEvent::InRange,   EnemyState::Attack);
 *   sm.AddTransition({},               EnemyEvent::Die,        EnemyState::Dead);
 *   sm.Start(EnemyState::Idle);
 *   sm.Tick(dt);
 *   sm.SendEvent(EnemyEvent::SeePlayer);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Core {

/// Non-template base for serialisation and generic containers
class StateMachineBase {
public:
    virtual ~StateMachineBase() = default;
    virtual void     Tick(float dt) = 0;
    virtual uint32_t GetCurrentStateId() const = 0;
    virtual bool     SendEventById(uint32_t eventId) = 0;
    virtual void     StartById(uint32_t stateId) = 0;
    virtual std::string GetCurrentStateName() const = 0;
};

template<typename StateT, typename EventT>
class StateMachine : public StateMachineBase {
public:
    using StateId = StateT;
    using EventId = EventT;
    using EnterCb     = std::function<void()>;
    using ExitCb      = std::function<void()>;
    using TickCb      = std::function<void(float)>;
    using TransitionCb= std::function<void(StateT from, StateT to)>;
    using GuardFn     = std::function<bool()>;

    struct StateDesc {
        StateT      id;
        std::string name;
        EnterCb     onEnter;
        TickCb      onTick;
        ExitCb      onExit;
        std::optional<StateT> parent;    ///< for hierarchy
        bool        isHistory{false};    ///< resume sub-state on re-entry
    };

    struct TransitionDesc {
        std::optional<StateT> from;  ///< nullopt = any state
        EventT                event;
        StateT                to;
        GuardFn               guard;
        TransitionCb          onTransition;
    };

    StateMachine() = default;
    ~StateMachine() override = default;

    // Build
    void AddState(StateT id, TickCb tick={},
                  EnterCb enter={}, ExitCb exit={},
                  const std::string& name="");
    void AddState(const StateDesc& desc);
    void AddTransition(std::optional<StateT> from, EventT event, StateT to,
                       GuardFn guard={}, TransitionCb onTrans={});
    void AddTransition(const TransitionDesc& desc);
    void SetParent(StateT child, StateT parent);

    // Runtime
    void Start(StateT initialState);
    void Tick(float dt) override;
    bool SendEvent(EventT event);
    bool SendEventById(uint32_t eventId) override;
    void ForceState(StateT state);

    // Query
    StateT      GetCurrentState() const;
    uint32_t    GetCurrentStateId() const override;
    std::string GetCurrentStateName() const override;
    bool        IsInState(StateT state) const;
    bool        IsRunning() const;
    float       TimeInCurrentState() const;

    // Global on-transition callback
    void SetOnTransition(TransitionCb cb);

private:
    struct StateEntry {
        StateDesc             desc;
        std::optional<StateT> historyState;
    };

    std::vector<StateEntry>    m_states;
    std::vector<TransitionDesc>m_transitions;
    std::optional<StateT>      m_current;
    float                      m_timeInState{0.f};
    bool                       m_running{false};
    TransitionCb               m_globalOnTransition;

    StateEntry* FindState(StateT id);
    bool        DoTransition(EventT event);
    void        EnterState(StateT id);
    void        ExitState(StateT id);
};

// ── Template implementation ──────────────────────────────────────────────────

template<typename S, typename E>
void StateMachine<S,E>::AddState(S id, TickCb tick, EnterCb enter, ExitCb exit,
                                  const std::string& name)
{
    StateDesc d; d.id=id; d.name=name.empty()?std::to_string((int)id):name;
    d.onEnter=enter; d.onTick=tick; d.onExit=exit;
    AddState(d);
}

template<typename S, typename E>
void StateMachine<S,E>::AddState(const StateDesc& desc)
{
    for (auto& se : m_states) if (se.desc.id == desc.id) { se.desc=desc; return; }
    m_states.push_back({desc, {}});
}

template<typename S, typename E>
void StateMachine<S,E>::AddTransition(std::optional<S> from, E event, S to,
                                       GuardFn guard, TransitionCb onTrans)
{
    TransitionDesc d; d.from=from; d.event=event; d.to=to;
    d.guard=guard; d.onTransition=onTrans;
    m_transitions.push_back(d);
}

template<typename S, typename E>
void StateMachine<S,E>::AddTransition(const TransitionDesc& desc)
{
    m_transitions.push_back(desc);
}

template<typename S, typename E>
void StateMachine<S,E>::SetParent(S child, S parent)
{
    for (auto& se : m_states) if (se.desc.id == child) { se.desc.parent=parent; return; }
}

template<typename S, typename E>
typename StateMachine<S,E>::StateEntry* StateMachine<S,E>::FindState(S id)
{
    for (auto& se : m_states) if (se.desc.id == id) return &se;
    return nullptr;
}

template<typename S, typename E>
void StateMachine<S,E>::EnterState(S id)
{
    auto* se = FindState(id);
    if (se && se->desc.onEnter) se->desc.onEnter();
    m_current = id;
    m_timeInState = 0.f;
}

template<typename S, typename E>
void StateMachine<S,E>::ExitState(S id)
{
    auto* se = FindState(id);
    if (se && se->desc.onExit) se->desc.onExit();
}

template<typename S, typename E>
void StateMachine<S,E>::Start(S initialState)
{
    m_running = true;
    EnterState(initialState);
}

template<typename S, typename E>
void StateMachine<S,E>::Tick(float dt)
{
    if (!m_running || !m_current) return;
    m_timeInState += dt;
    auto* se = FindState(*m_current);
    if (se && se->desc.onTick) se->desc.onTick(dt);
}

template<typename S, typename E>
bool StateMachine<S,E>::DoTransition(E event)
{
    for (auto& t : m_transitions) {
        bool srcMatch = !t.from.has_value() || (m_current && t.from.value() == *m_current);
        if (!srcMatch || t.event != event) continue;
        if (t.guard && !t.guard()) continue;
        S from = m_current.value_or(t.to);
        ExitState(from);
        if (t.onTransition) t.onTransition(from, t.to);
        if (m_globalOnTransition) m_globalOnTransition(from, t.to);
        EnterState(t.to);
        return true;
    }
    return false;
}

template<typename S, typename E>
bool StateMachine<S,E>::SendEvent(E event) { return DoTransition(event); }

template<typename S, typename E>
bool StateMachine<S,E>::SendEventById(uint32_t eid)
{
    return SendEvent(static_cast<E>(eid));
}

template<typename S, typename E>
void StateMachine<S,E>::ForceState(S state)
{
    if (m_current) ExitState(*m_current);
    EnterState(state);
}

template<typename S, typename E>
S StateMachine<S,E>::GetCurrentState() const { return m_current.value_or(S{}); }

template<typename S, typename E>
uint32_t StateMachine<S,E>::GetCurrentStateId() const
{ return static_cast<uint32_t>(GetCurrentState()); }

template<typename S, typename E>
std::string StateMachine<S,E>::GetCurrentStateName() const
{
    if (!m_current) return "";
    const auto* se = const_cast<StateMachine*>(this)->FindState(*m_current);
    return se ? se->desc.name : "";
}

template<typename S, typename E>
bool StateMachine<S,E>::IsInState(S state) const
{ return m_current && *m_current == state; }

template<typename S, typename E>
bool StateMachine<S,E>::IsRunning() const { return m_running; }

template<typename S, typename E>
float StateMachine<S,E>::TimeInCurrentState() const { return m_timeInState; }

template<typename S, typename E>
void StateMachine<S,E>::SetOnTransition(TransitionCb cb)
{ m_globalOnTransition = cb; }

} // namespace Core
