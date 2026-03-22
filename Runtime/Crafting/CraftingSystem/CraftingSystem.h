#pragma once
/**
 * @file CraftingSystem.h
 * @brief Recipe-based crafting with ingredients, sessions, and callbacks.
 *
 * CraftingSystem manages a registry of recipes and active crafting sessions:
 *   - RecipeDesc: list of ingredient (id, quantity) inputs and outputs,
 *     station type required, craft time in seconds, and minimum skill level.
 *   - AddRecipe(desc): register a recipe; returns stable RecipeId.
 *   - RemoveRecipe(id): unregister a recipe.
 *   - StartCraft(recipeId, inventory): begin a timed crafting session.
 *   - CancelCraft(sessionId): abort an in-progress session.
 *   - CraftInstant(recipeId): immediate single-step craft (no time).
 *   - Tick(dt): advance all active sessions; fires completion callbacks.
 *   - GetAvailableRecipes(inventory): filter by ingredient availability.
 *   - Callbacks: OnCraftComplete, OnCraftFailed, OnCraftProgress.
 *   - CraftingStats: totalRecipes, activeSessions, completedTotal, failedTotal.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace Runtime {

// ── Typed handles ─────────────────────────────────────────────────────────
using IngredientId = uint32_t;
using RecipeId     = uint32_t;
using SessionId    = uint32_t;
static constexpr RecipeId  kInvalidRecipeId  = 0;
static constexpr SessionId kInvalidSessionId = 0;

// ── Ingredient slot ───────────────────────────────────────────────────────
struct Ingredient {
    IngredientId id{0};
    uint32_t     quantity{1};
};

// ── Recipe descriptor ─────────────────────────────────────────────────────
struct RecipeDesc {
    std::string           name;
    std::vector<Ingredient> inputs;
    std::vector<Ingredient> outputs;
    std::string           stationType;   ///< e.g. "Workbench", "Forge", ""
    float                 craftTimeSec{1.0f}; ///< 0 = instant
    uint32_t              skillRequired{0};
    std::string           category;
    std::string           description;
};

// ── Live crafting session ─────────────────────────────────────────────────
struct CraftSession {
    SessionId   sessionId{kInvalidSessionId};
    RecipeId    recipeId{kInvalidRecipeId};
    float       elapsed{0.0f};    ///< Seconds elapsed
    float       total{0.0f};      ///< Total craft time
    bool        active{false};
    float       Progress() const { return total > 0.0f ? elapsed / total : 1.0f; }
};

// ── Inventory query interface ─────────────────────────────────────────────
/// Callers provide a function that returns how many of an ingredient are held.
using InventoryQueryFn = std::function<uint32_t(IngredientId)>;
/// Callers provide a function that consumes ingredients from inventory.
using InventoryConsumeFn = std::function<bool(const std::vector<Ingredient>&)>;
/// Callers provide a function that adds outputs to inventory.
using InventoryAddFn = std::function<void(const std::vector<Ingredient>&)>;

// ── Callbacks ─────────────────────────────────────────────────────────────
using CraftCompleteCb = std::function<void(SessionId, RecipeId)>;
using CraftFailedCb   = std::function<void(SessionId, RecipeId, const std::string& reason)>;
using CraftProgressCb = std::function<void(SessionId, float progress)>;

// ── Stats ─────────────────────────────────────────────────────────────────
struct CraftingStats {
    uint32_t totalRecipes{0};
    uint32_t activeSessions{0};
    uint32_t completedTotal{0};
    uint32_t failedTotal{0};
    uint32_t completedThisFrame{0};
};

// ── CraftingSystem ────────────────────────────────────────────────────────
class CraftingSystem {
public:
    CraftingSystem();
    ~CraftingSystem();

    CraftingSystem(const CraftingSystem&) = delete;
    CraftingSystem& operator=(const CraftingSystem&) = delete;

    // ── recipe registry ───────────────────────────────────────
    RecipeId      AddRecipe(const RecipeDesc& desc);
    bool          RemoveRecipe(RecipeId id);
    const RecipeDesc* GetRecipe(RecipeId id) const;
    std::vector<RecipeId> AllRecipeIds() const;

    /// Returns recipes whose inputs can be satisfied by the given inventory.
    std::vector<RecipeId> GetAvailableRecipes(const InventoryQueryFn& inv) const;
    /// Returns recipes matching the station type.
    std::vector<RecipeId> GetRecipesByStation(const std::string& station) const;
    /// Returns recipes in the given category.
    std::vector<RecipeId> GetRecipesByCategory(const std::string& category) const;

    // ── crafting sessions ─────────────────────────────────────
    /**
     * Start a crafting session.
     * @param recipeId   Recipe to craft.
     * @param query      Inventory query to check ingredient availability.
     * @param consume    Called on session start to deduct ingredients.
     * @param add        Called on session complete to give outputs.
     * @return           New SessionId, or kInvalidSessionId on failure.
     */
    SessionId StartCraft(RecipeId recipeId,
                         const InventoryQueryFn&  query,
                         const InventoryConsumeFn& consume,
                         const InventoryAddFn&    add);

    bool CancelCraft(SessionId sessionId);

    /**
     * Instant craft — no time, no callbacks, immediate result.
     * Returns true on success (ingredients consumed, outputs delivered).
     */
    bool CraftInstant(RecipeId recipeId,
                      const InventoryQueryFn&  query,
                      const InventoryConsumeFn& consume,
                      const InventoryAddFn&    add);

    // ── per-frame ─────────────────────────────────────────────
    /// Advance all active sessions by dt seconds.
    void Tick(float dt);

    // ── session queries ───────────────────────────────────────
    const CraftSession* GetSession(SessionId id) const;
    std::vector<SessionId> ActiveSessions() const;

    // ── callbacks ─────────────────────────────────────────────
    void OnCraftComplete(CraftCompleteCb cb);
    void OnCraftFailed(CraftFailedCb cb);
    void OnCraftProgress(CraftProgressCb cb);  ///< fired every Tick for active sessions

    // ── stats ─────────────────────────────────────────────────
    CraftingStats Stats() const;
    void          ResetStats();

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
