#pragma once
/**
 * @file InteractionSystem.h
 * @brief Player / NPC interaction system — proximity detection, interaction prompts, callbacks.
 *
 * Interactable objects register with the system.  Each frame the system
 * queries proximity to an "interactor" (e.g. the player), builds a list of
 * candidates sorted by distance/priority, and exposes the best candidate for
 * UI prompt display.  On Interact(), the registered callback fires.
 *
 * Features:
 *   - Range-based or ray-cast interaction modes
 *   - Priority / weight override per interactable
 *   - One-shot and repeatable interactions with cooldown
 *   - Interaction categories (use, talk, pick-up, unlock…)
 *   - Multiple concurrent interactors (split-screen co-op)
 *
 * Typical usage:
 * @code
 *   InteractionSystem is;
 *   is.Init();
 *   uint32_t id = is.Register({"chest_01", {10,0,5}, InteractCategory::Use,
 *                               []{ OpenChest(); }});
 *   is.SetInteractorPosition(0, playerPos);
 *   is.Update(dt);
 *   if (is.HasCandidate(0)) {
 *       ShowPrompt(is.GetBestCandidate(0));
 *       if (playerPressedE) is.Interact(0);
 *   }
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class InteractCategory : uint8_t {
    Use=0, Talk, PickUp, Unlock, Examine, Activate, Other
};

struct InteractableDesc {
    std::string        id;
    float              position[3]{};
    InteractCategory   category{InteractCategory::Use};
    std::string        prompt;          ///< e.g. "[E] Open"
    float              range{2.f};
    float              cooldown{0.f};   ///< 0 = instant repeat allowed
    int32_t            priority{0};     ///< higher = preferred
    bool               oneShot{false};  ///< removed after first use
    bool               enabled{true};
    std::function<void(uint32_t interactorId)> onInteract;
};

struct InteractableState {
    uint32_t      handle{0};
    InteractableDesc desc;
    float         cooldownRemaining{0.f};
    uint32_t      useCount{0};
    bool          alive{true};
};

class InteractionSystem {
public:
    InteractionSystem();
    ~InteractionSystem();

    void Init();
    void Shutdown();

    // Registration
    uint32_t Register(const InteractableDesc& desc);
    void     Unregister(uint32_t handle);
    bool     HasInteractable(uint32_t handle) const;
    void     SetEnabled(uint32_t handle, bool enabled);
    void     SetPosition(uint32_t handle, const float pos[3]);
    InteractableState GetState(uint32_t handle) const;

    // Interactor management (player or AI agent)
    void SetInteractorPosition(uint32_t interactorId, const float pos[3]);
    void SetInteractorRange(uint32_t interactorId, float range);

    // Query
    bool     HasCandidate(uint32_t interactorId) const;
    uint32_t GetBestCandidateHandle(uint32_t interactorId) const;
    InteractableDesc GetBestCandidate(uint32_t interactorId) const;
    std::vector<InteractableState> GetCandidates(uint32_t interactorId) const;

    // Action
    bool Interact(uint32_t interactorId);
    bool InteractWith(uint32_t interactorId, uint32_t handle);

    void Update(float dt);

    void OnInteract(std::function<void(uint32_t interactorId, uint32_t handle)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Runtime
