#include "Runtime/Crafting/CraftingSystem/CraftingSystem.h"
#include <algorithm>
#include <stdexcept>

namespace Runtime {

// ── Impl ──────────────────────────────────────────────────────────────────
struct CraftingSystem::Impl {
    // Recipe registry
    std::unordered_map<RecipeId, RecipeDesc> recipes;
    RecipeId nextRecipeId{1};

    // Active sessions
    struct ActiveSession {
        CraftSession session;
        InventoryAddFn addFn;
    };
    std::unordered_map<SessionId, ActiveSession> sessions;
    SessionId nextSessionId{1};

    // Callbacks
    std::vector<CraftCompleteCb>  completeCbs;
    std::vector<CraftFailedCb>    failedCbs;
    std::vector<CraftProgressCb>  progressCbs;

    // Stats
    uint32_t completedTotal{0};
    uint32_t failedTotal{0};
    uint32_t completedThisFrame{0};

    bool HasIngredients(const RecipeDesc& r, const InventoryQueryFn& query) const {
        for (const auto& ing : r.inputs) {
            if (query(ing.id) < ing.quantity) return false;
        }
        return true;
    }
};

// ── Lifecycle ─────────────────────────────────────────────────────────────
CraftingSystem::CraftingSystem() : m_impl(new Impl()) {}
CraftingSystem::~CraftingSystem() { delete m_impl; }

// ── Recipe registry ───────────────────────────────────────────────────────
RecipeId CraftingSystem::AddRecipe(const RecipeDesc& desc) {
    RecipeId id = m_impl->nextRecipeId++;
    m_impl->recipes[id] = desc;
    return id;
}

bool CraftingSystem::RemoveRecipe(RecipeId id) {
    return m_impl->recipes.erase(id) > 0;
}

const RecipeDesc* CraftingSystem::GetRecipe(RecipeId id) const {
    auto it = m_impl->recipes.find(id);
    return it != m_impl->recipes.end() ? &it->second : nullptr;
}

std::vector<RecipeId> CraftingSystem::AllRecipeIds() const {
    std::vector<RecipeId> out;
    out.reserve(m_impl->recipes.size());
    for (const auto& [id, _] : m_impl->recipes) out.push_back(id);
    return out;
}

std::vector<RecipeId> CraftingSystem::GetAvailableRecipes(const InventoryQueryFn& inv) const {
    std::vector<RecipeId> out;
    for (const auto& [id, r] : m_impl->recipes) {
        if (m_impl->HasIngredients(r, inv)) out.push_back(id);
    }
    return out;
}

std::vector<RecipeId> CraftingSystem::GetRecipesByStation(const std::string& station) const {
    std::vector<RecipeId> out;
    for (const auto& [id, r] : m_impl->recipes) {
        if (r.stationType == station) out.push_back(id);
    }
    return out;
}

std::vector<RecipeId> CraftingSystem::GetRecipesByCategory(const std::string& category) const {
    std::vector<RecipeId> out;
    for (const auto& [id, r] : m_impl->recipes) {
        if (r.category == category) out.push_back(id);
    }
    return out;
}

// ── Crafting sessions ─────────────────────────────────────────────────────
SessionId CraftingSystem::StartCraft(RecipeId recipeId,
                                      const InventoryQueryFn&  query,
                                      const InventoryConsumeFn& consume,
                                      const InventoryAddFn&    add) {
    auto it = m_impl->recipes.find(recipeId);
    if (it == m_impl->recipes.end()) return kInvalidSessionId;
    const RecipeDesc& r = it->second;
    if (!m_impl->HasIngredients(r, query)) return kInvalidSessionId;
    if (!consume(r.inputs)) return kInvalidSessionId;

    SessionId sid = m_impl->nextSessionId++;
    Impl::ActiveSession as;
    as.session.sessionId = sid;
    as.session.recipeId  = recipeId;
    as.session.elapsed   = 0.0f;
    as.session.total     = r.craftTimeSec;
    as.session.active    = true;
    as.addFn = add;
    m_impl->sessions[sid] = std::move(as);
    return sid;
}

bool CraftingSystem::CancelCraft(SessionId sessionId) {
    auto it = m_impl->sessions.find(sessionId);
    if (it == m_impl->sessions.end()) return false;
    for (auto& cb : m_impl->failedCbs)
        cb(sessionId, it->second.session.recipeId, "Cancelled");
    ++m_impl->failedTotal;
    m_impl->sessions.erase(it);
    return true;
}

bool CraftingSystem::CraftInstant(RecipeId recipeId,
                                   const InventoryQueryFn&  query,
                                   const InventoryConsumeFn& consume,
                                   const InventoryAddFn&    add) {
    auto it = m_impl->recipes.find(recipeId);
    if (it == m_impl->recipes.end()) return false;
    const RecipeDesc& r = it->second;
    if (!m_impl->HasIngredients(r, query)) return false;
    if (!consume(r.inputs)) return false;
    add(r.outputs);
    ++m_impl->completedTotal;
    return true;
}

// ── Tick ──────────────────────────────────────────────────────────────────
void CraftingSystem::Tick(float dt) {
    m_impl->completedThisFrame = 0;
    std::vector<SessionId> finished;

    for (auto& [sid, as] : m_impl->sessions) {
        if (!as.session.active) continue;
        as.session.elapsed += dt;

        // Fire progress callbacks
        float prog = as.session.Progress();
        for (auto& cb : m_impl->progressCbs) cb(sid, prog);

        if (as.session.elapsed >= as.session.total) {
            // Complete
            as.addFn(m_impl->recipes.at(as.session.recipeId).outputs);
            for (auto& cb : m_impl->completeCbs) cb(sid, as.session.recipeId);
            ++m_impl->completedTotal;
            ++m_impl->completedThisFrame;
            finished.push_back(sid);
        }
    }
    for (SessionId sid : finished) m_impl->sessions.erase(sid);
}

// ── Session queries ───────────────────────────────────────────────────────
const CraftSession* CraftingSystem::GetSession(SessionId id) const {
    auto it = m_impl->sessions.find(id);
    return it != m_impl->sessions.end() ? &it->second.session : nullptr;
}

std::vector<SessionId> CraftingSystem::ActiveSessions() const {
    std::vector<SessionId> out;
    out.reserve(m_impl->sessions.size());
    for (const auto& [sid, _] : m_impl->sessions) out.push_back(sid);
    return out;
}

// ── Callbacks ─────────────────────────────────────────────────────────────
void CraftingSystem::OnCraftComplete(CraftCompleteCb cb)  { m_impl->completeCbs.push_back(std::move(cb)); }
void CraftingSystem::OnCraftFailed(CraftFailedCb cb)      { m_impl->failedCbs.push_back(std::move(cb)); }
void CraftingSystem::OnCraftProgress(CraftProgressCb cb)  { m_impl->progressCbs.push_back(std::move(cb)); }

// ── Stats ─────────────────────────────────────────────────────────────────
CraftingStats CraftingSystem::Stats() const {
    CraftingStats s;
    s.totalRecipes       = static_cast<uint32_t>(m_impl->recipes.size());
    s.activeSessions     = static_cast<uint32_t>(m_impl->sessions.size());
    s.completedTotal     = m_impl->completedTotal;
    s.failedTotal        = m_impl->failedTotal;
    s.completedThisFrame = m_impl->completedThisFrame;
    return s;
}

void CraftingSystem::ResetStats() {
    m_impl->completedTotal     = 0;
    m_impl->failedTotal        = 0;
    m_impl->completedThisFrame = 0;
}

} // namespace Runtime
