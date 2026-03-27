#pragma once
/**
 * @file QuestSystem.h
 * @brief Quest/objective tree, prerequisite chains, reward dispatch, JSON save/load.
 *
 * Features:
 *   - Quest definition: id, title, description, category, repeatable
 *   - Objective list per quest (ordered or parallel)
 *   - Objective types: Kill, Collect, Reach, Interact, Custom
 *   - Prerequisite chains: quest A requires quest B completed
 *   - Quest states: Inactive, Active, Completed, Failed, Abandoned
 *   - Progress tracking: per-objective progress / goal
 *   - Reward dispatch: call reward callback on complete
 *   - Time limit per quest (optional)
 *   - On-complete / on-fail / on-activate callbacks
 *   - Serialise/Deserialise all quest states to JSON
 *   - Tick: advance timers, fail expired quests
 *
 * Typical usage:
 * @code
 *   QuestSystem qs;
 *   qs.Init();
 *   qs.Register({"slay_wolves","Slay 5 Wolves","Hunt wolves","Main",
 *                {{"kill_wolves","Kill", 5}}, {}, false});
 *   qs.SetOnComplete([](const std::string& id){ GiveReward(id); });
 *   qs.Activate("slay_wolves");
 *   qs.AddObjectiveProgress("slay_wolves","kill_wolves", 1);
 *   qs.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class QuestState : uint8_t {
    Inactive=0, Active, Completed, Failed, Abandoned
};
enum class ObjectiveType : uint8_t {
    Kill=0, Collect, Reach, Interact, Escort, Custom
};

struct ObjectiveDef {
    std::string   id;
    ObjectiveType type {ObjectiveType::Kill};
    uint32_t      goal {1};
    std::string   description;
    bool          optional{false};
};

struct QuestDef {
    std::string                  id;
    std::string                  title;
    std::string                  description;
    std::string                  category;
    std::vector<ObjectiveDef>    objectives;
    std::vector<std::string>     prerequisites; ///< quest ids that must be complete first
    bool                         repeatable{false};
    float                        timeLimit{0.f};///< 0 = no limit
};

struct ObjectiveState {
    std::string id;
    uint32_t    progress{0};
    bool        complete{false};
};

struct QuestState_ {            ///< renamed to avoid clash with enum
    std::string                  questId;
    QuestState                   state{QuestState::Inactive};
    std::vector<ObjectiveState>  objectives;
    float                        timeRemaining{0.f};
};

class QuestSystem {
public:
    QuestSystem();
    ~QuestSystem();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    // Definitions
    void Register  (const QuestDef& def);
    bool Has       (const std::string& questId) const;
    const QuestDef* Get(const std::string& questId) const;
    std::vector<QuestDef> GetAll()                        const;
    std::vector<QuestDef> GetByCategory(const std::string& cat) const;

    // Lifecycle
    bool Activate  (const std::string& questId);
    bool Abandon   (const std::string& questId);
    bool Fail      (const std::string& questId);
    bool Complete  (const std::string& questId);

    // Prerequisite check
    bool CanActivate(const std::string& questId) const;

    // Progress
    void     AddObjectiveProgress(const std::string& questId,
                                   const std::string& objectiveId, uint32_t delta=1);
    void     SetObjectiveProgress(const std::string& questId,
                                   const std::string& objectiveId, uint32_t value);
    uint32_t GetObjectiveProgress(const std::string& questId,
                                   const std::string& objectiveId) const;
    bool     IsObjectiveComplete (const std::string& questId,
                                   const std::string& objectiveId) const;

    // State queries
    QuestState              GetState   (const std::string& questId) const;
    const QuestState_*      GetQState  (const std::string& questId) const;
    std::vector<std::string> GetActive () const;
    std::vector<std::string> GetCompleted() const;
    float CompletionPct() const;

    // Callbacks
    void SetOnActivate (std::function<void(const std::string& id)> cb);
    void SetOnComplete (std::function<void(const std::string& id)> cb);
    void SetOnFail     (std::function<void(const std::string& id)> cb);
    void SetOnProgress (std::function<void(const std::string& questId,
                                            const std::string& objId,
                                            uint32_t progress)> cb);

    // Persistence
    std::string Serialize()              const;
    bool        Deserialize(const std::string& json);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
