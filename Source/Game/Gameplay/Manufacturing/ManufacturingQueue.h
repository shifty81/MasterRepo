#pragma once
#include "Game/Gameplay/Economy/ResourceRegistry.h"
#include "Game/Interaction/ResourceItem.h"
#include "Game/Interaction/Inventory.h"
#include <cstdint>
#include <vector>
#include <array>

namespace NF::Game::Gameplay {

/// @brief One input ingredient in a craft recipe.
struct Ingredient {
    NF::Game::ResourceType type{NF::Game::ResourceType::None};
    uint32_t               count{0};
};

/// @brief A craft recipe: N inputs → 1 output item.
struct CraftRecipe {
    static constexpr int kMaxIngredients = 4;

    NF::Game::ResourceType output{NF::Game::ResourceType::None};
    uint32_t               outputCount{1};
    float                  duration{5.f}; ///< Seconds to craft one batch.
    std::array<Ingredient, kMaxIngredients> inputs{};
    int                    ingredientCount{0};
};

/// @brief Status of an active craft job.
enum class JobStatus : uint8_t {
    Queued,      ///< Waiting for the previous job to finish.
    InProgress,  ///< Currently being crafted.
    Completed,   ///< Finished — call CollectOutput() to retrieve results.
};

/// @brief Handle to an enqueued manufacturing job.
using JobId = uint32_t;
static constexpr JobId kInvalidJobId = 0;

/// @brief Active job record.
struct ManufacturingJob {
    JobId      id{kInvalidJobId};
    CraftRecipe recipe;
    float      elapsed{0.f};  ///< Seconds of work accumulated so far.
    JobStatus  status{JobStatus::Queued};
};

/// @brief Recipe-based crafting pipeline.
///
/// A single queue of jobs; one job runs at a time.  Resources are deducted
/// from the Inventory when a job is enqueued.  When a job completes, the
/// output items sit in the queue until CollectOutput() deposits them into
/// the Inventory.
///
/// Call Tick(dt) each frame to advance the active job.
class ManufacturingQueue {
public:
    ManufacturingQueue();
    ~ManufacturingQueue() = default;

    // -------------------------------------------------------------------------
    // Recipe catalog
    // -------------------------------------------------------------------------

    /// @brief Number of built-in recipes.
    [[nodiscard]] int RecipeCount() const noexcept { return m_RecipeCount; }

    /// @brief Return a pointer to the recipe for @p output, or nullptr.
    [[nodiscard]] const CraftRecipe* FindRecipe(NF::Game::ResourceType output) const noexcept;

    // -------------------------------------------------------------------------
    // Enqueue
    // -------------------------------------------------------------------------

    /// @brief Enqueue a crafting job for @p output.
    ///
    /// Deducts ingredients from @p inv immediately.
    /// @return The job id on success, or kInvalidJobId if ingredients are
    ///         unavailable, no recipe exists, or the queue is at capacity.
    JobId Enqueue(NF::Game::ResourceType output, NF::Game::Inventory& inv);

    // -------------------------------------------------------------------------
    // Tick
    // -------------------------------------------------------------------------

    /// @brief Advance the front-of-queue job by @p dt seconds.
    ///        Marks it Completed when elapsed >= recipe.duration.
    void Tick(float dt);

    // -------------------------------------------------------------------------
    // Output collection
    // -------------------------------------------------------------------------

    /// @brief True if the front job is completed and waiting for collection.
    [[nodiscard]] bool HasCompletedJob() const noexcept;

    /// @brief Deposit the completed job's output into @p inv and remove it from queue.
    /// @return False if no completed job is available.
    bool CollectOutput(NF::Game::Inventory& inv);

    // -------------------------------------------------------------------------
    // Queue state
    // -------------------------------------------------------------------------

    [[nodiscard]] int  PendingCount()  const noexcept { return static_cast<int>(m_Jobs.size()); }
    [[nodiscard]] bool IsIdle()        const noexcept { return m_Jobs.empty(); }

private:
    static constexpr int kMaxRecipes = 4;

    CraftRecipe m_Recipes[kMaxRecipes];
    int         m_RecipeCount{0};
    JobId       m_NextJobId{1};
    std::vector<ManufacturingJob> m_Jobs;
};

} // namespace NF::Game::Gameplay
