#include "Game/Gameplay/Manufacturing/ManufacturingQueue.h"

namespace NF::Game::Gameplay {

ManufacturingQueue::ManufacturingQueue() {
    // Recipe: Stone + Ore → Metal (1 unit, 8 s)
    {
        CraftRecipe r;
        r.output        = NF::Game::ResourceType::Metal;
        r.outputCount   = 1;
        r.duration      = 8.f;
        r.inputs[0]     = { NF::Game::ResourceType::Stone, 2 };
        r.inputs[1]     = { NF::Game::ResourceType::Ore,   3 };
        r.ingredientCount = 2;
        m_Recipes[m_RecipeCount++] = r;
    }
    // Recipe: Organic + Ice → Organic (processed, 2 units, 5 s)
    {
        CraftRecipe r;
        r.output        = NF::Game::ResourceType::Organic;
        r.outputCount   = 2;
        r.duration      = 5.f;
        r.inputs[0]     = { NF::Game::ResourceType::Organic, 3 };
        r.inputs[1]     = { NF::Game::ResourceType::Ice,     1 };
        r.ingredientCount = 2;
        m_Recipes[m_RecipeCount++] = r;
    }
    // Recipe: Rock + Metal → Metal (refined, 2 units, 12 s)
    {
        CraftRecipe r;
        r.output        = NF::Game::ResourceType::Metal;
        r.outputCount   = 2;
        r.duration      = 12.f;
        r.inputs[0]     = { NF::Game::ResourceType::Rock,  4 };
        r.inputs[1]     = { NF::Game::ResourceType::Metal, 1 };
        r.ingredientCount = 2;
        m_Recipes[m_RecipeCount++] = r;
    }
    // Recipe: Ore → Stone (smelted, 3 units, 3 s)
    {
        CraftRecipe r;
        r.output        = NF::Game::ResourceType::Stone;
        r.outputCount   = 3;
        r.duration      = 3.f;
        r.inputs[0]     = { NF::Game::ResourceType::Ore, 2 };
        r.ingredientCount = 1;
        m_Recipes[m_RecipeCount++] = r;
    }
}

const CraftRecipe* ManufacturingQueue::FindRecipe(NF::Game::ResourceType output) const noexcept {
    for (int i = 0; i < m_RecipeCount; ++i) {
        if (m_Recipes[i].output == output) return &m_Recipes[i];
    }
    return nullptr;
}

JobId ManufacturingQueue::Enqueue(NF::Game::ResourceType output, NF::Game::Inventory& inv) {
    const CraftRecipe* recipe = FindRecipe(output);
    if (!recipe) return kInvalidJobId;

    // Check that all ingredients are available.
    for (int i = 0; i < recipe->ingredientCount; ++i) {
        if (!inv.HasItem(recipe->inputs[static_cast<size_t>(i)].type,
                          recipe->inputs[static_cast<size_t>(i)].count))
            return kInvalidJobId;
    }

    // Deduct ingredients immediately.
    for (int i = 0; i < recipe->ingredientCount; ++i) {
        inv.RemoveItem(recipe->inputs[static_cast<size_t>(i)].type,
                        recipe->inputs[static_cast<size_t>(i)].count);
    }

    ManufacturingJob job;
    job.id     = m_NextJobId++;
    job.recipe = *recipe;
    job.status = m_Jobs.empty() ? JobStatus::InProgress : JobStatus::Queued;
    m_Jobs.push_back(job);
    return job.id;
}

void ManufacturingQueue::Tick(float dt) {
    if (m_Jobs.empty()) return;

    ManufacturingJob& front = m_Jobs.front();
    if (front.status == JobStatus::Completed) return; // waiting for collection

    front.status   = JobStatus::InProgress;
    front.elapsed += dt;
    if (front.elapsed >= front.recipe.duration)
        front.status = JobStatus::Completed;
}

bool ManufacturingQueue::HasCompletedJob() const noexcept {
    return !m_Jobs.empty() && m_Jobs.front().status == JobStatus::Completed;
}

bool ManufacturingQueue::CollectOutput(NF::Game::Inventory& inv) {
    if (!HasCompletedJob()) return false;

    const ManufacturingJob& job = m_Jobs.front();
    inv.AddItem(job.recipe.output, job.recipe.outputCount);
    m_Jobs.erase(m_Jobs.begin());

    // Activate the next job if one is waiting.
    if (!m_Jobs.empty() && m_Jobs.front().status == JobStatus::Queued)
        m_Jobs.front().status = JobStatus::InProgress;

    return true;
}

} // namespace NF::Game::Gameplay
