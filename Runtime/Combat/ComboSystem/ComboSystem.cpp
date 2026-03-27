#include "Runtime/Combat/ComboSystem/ComboSystem.h"
#include <algorithm>
#include <fstream>
#include <unordered_map>

namespace Runtime {

struct FighterState {
    uint32_t              handle{0};
    uint32_t              entityId{0};
    std::string           comboDefId;
    uint32_t              currentNodeId{0};     ///< 0 = idle
    float                 windowTimer{0.f};
    bool                  awaitingInput{false};
    bool                  hasPendingAttack{false};
    AttackResult          pendingAttack;
    FighterStats          stats;
    std::vector<uint32_t> currentChain;
};

struct ComboSystem::Impl {
    std::unordered_map<std::string, ComboDefinition> defs;
    std::unordered_map<uint32_t, FighterState>       fighters;
    uint32_t nextHandle{1};

    ComboBreakCb onBreak;
    FinisherCb   onFinisher;

    const ComboNode* FindNode(const ComboDefinition& def, uint32_t nodeId) const {
        for (auto& n : def.nodes) if (n.nodeId == nodeId) return &n;
        return nullptr;
    }

    const ComboNode* FindFollowUp(const ComboDefinition& def,
                                   uint32_t currentId,
                                   const std::string& input) const {
        const auto* cur = FindNode(def, currentId);
        if (!cur) {
            // idle → try roots
            for (auto rootId : def.rootNodes) {
                const auto* rn = FindNode(def, rootId);
                if (rn && rn->inputRequired == input) return rn;
            }
            return nullptr;
        }
        for (auto fid : cur->followUps) {
            const auto* fn = FindNode(def, fid);
            if (fn && fn->inputRequired == input) return fn;
        }
        return nullptr;
    }

    void BreakCombo(FighterState& fs) {
        if (!fs.currentChain.empty() && onBreak)
            onBreak(fs.handle, (uint32_t)fs.currentChain.size());
        if (fs.stats.currentComboLength > fs.stats.longestCombo)
            fs.stats.longestCombo = fs.stats.currentComboLength;
        fs.stats.currentComboLength = 0;
        fs.currentChain.clear();
        fs.currentNodeId = 0;
        fs.awaitingInput = false;
        fs.windowTimer   = 0.f;
    }
};

ComboSystem::ComboSystem()  : m_impl(new Impl) {}
ComboSystem::~ComboSystem() { Shutdown(); delete m_impl; }

void ComboSystem::Init()     {}
void ComboSystem::Shutdown() { m_impl->fighters.clear(); }

void ComboSystem::Tick(float dt)
{
    for (auto& [h, fs] : m_impl->fighters) {
        if (!fs.awaitingInput) continue;
        fs.windowTimer += dt;

        auto dit = m_impl->defs.find(fs.comboDefId);
        if (dit == m_impl->defs.end()) continue;
        const auto* cur = m_impl->FindNode(dit->second, fs.currentNodeId);
        if (!cur) continue;

        if (fs.windowTimer > cur->linkWindow.lateThresh) {
            m_impl->BreakCombo(fs);
        }
    }
}

void ComboSystem::LoadDefinition(const ComboDefinition& def)
{
    m_impl->defs[def.id] = def;
}

bool ComboSystem::LoadFromJSON(const std::string& /*path*/) { return true; }

uint32_t ComboSystem::RegisterFighter(uint32_t entityId, const std::string& comboDefId)
{
    uint32_t h = m_impl->nextHandle++;
    FighterState fs;
    fs.handle = h;
    fs.entityId = entityId;
    fs.comboDefId = comboDefId;
    m_impl->fighters[h] = fs;
    return h;
}

void ComboSystem::UnregisterFighter(uint32_t handle)
{
    m_impl->fighters.erase(handle);
}

void ComboSystem::SetComboDefinition(uint32_t handle, const std::string& comboDefId)
{
    auto it = m_impl->fighters.find(handle);
    if (it != m_impl->fighters.end()) it->second.comboDefId = comboDefId;
}

void ComboSystem::FeedInput(uint32_t handle, const std::string& inputName)
{
    auto fit = m_impl->fighters.find(handle);
    if (fit == m_impl->fighters.end()) return;
    auto& fs = fit->second;

    auto dit = m_impl->defs.find(fs.comboDefId);
    if (dit == m_impl->defs.end()) return;

    const ComboNode* next = m_impl->FindFollowUp(dit->second, fs.currentNodeId, inputName);
    if (!next) {
        m_impl->BreakCombo(fs);
        // Try as new chain start
        next = m_impl->FindFollowUp(dit->second, 0, inputName);
        if (!next) return;
    }

    // Determine link quality
    ComboLinkQuality q = ComboLinkQuality::Perfect;
    if (fs.awaitingInput) {
        const auto* cur = m_impl->FindNode(dit->second, fs.currentNodeId);
        if (cur) {
            if (fs.windowTimer < cur->linkWindow.earlyThresh)
                q = ComboLinkQuality::Early;
            else if (fs.windowTimer < cur->linkWindow.perfectStart)
                q = ComboLinkQuality::Early;
            else if (fs.windowTimer <= cur->linkWindow.perfectEnd)
                q = ComboLinkQuality::Perfect;
            else
                q = ComboLinkQuality::Late;
        }
    }

    float dmgMult = next->damageMultiplier;
    if (q == ComboLinkQuality::Perfect) {
        const auto* prev = m_impl->FindNode(dit->second, fs.currentNodeId);
        if (prev) dmgMult *= prev->linkWindow.perfectBonus;
    }

    fs.currentNodeId = next->nodeId;
    fs.windowTimer   = 0.f;
    fs.awaitingInput = true;
    fs.currentChain.push_back(next->nodeId);
    fs.stats.currentComboLength++;
    fs.stats.totalHits++;
    fs.stats.timeSinceLastHit = 0.f;

    AttackResult ar;
    ar.moveName         = next->moveName;
    ar.damageMultiplier = dmgMult;
    ar.linkQuality      = q;
    ar.isFinisher       = next->isFinisher;
    ar.comboCount       = fs.stats.currentComboLength;

    fs.hasPendingAttack = true;
    fs.pendingAttack    = ar;

    if (next->isFinisher && m_impl->onFinisher)
        m_impl->onFinisher(handle, ar);
}

void ComboSystem::Break(uint32_t handle)
{
    auto it = m_impl->fighters.find(handle);
    if (it != m_impl->fighters.end()) m_impl->BreakCombo(it->second);
}

void ComboSystem::OnMoveComplete(uint32_t handle)
{
    auto it = m_impl->fighters.find(handle);
    if (it == m_impl->fighters.end()) return;
    auto& fs = it->second;
    if (!fs.awaitingInput) return;
    // Let Tick handle timeout
}

bool ComboSystem::HasPendingAttack(uint32_t handle) const
{
    auto it = m_impl->fighters.find(handle);
    return it != m_impl->fighters.end() && it->second.hasPendingAttack;
}

AttackResult ComboSystem::ConsumeAttack(uint32_t handle)
{
    auto it = m_impl->fighters.find(handle);
    if (it == m_impl->fighters.end()) return {};
    auto& fs = it->second;
    fs.hasPendingAttack = false;
    return fs.pendingAttack;
}

FighterStats ComboSystem::GetStats(uint32_t handle) const
{
    auto it = m_impl->fighters.find(handle);
    return it != m_impl->fighters.end() ? it->second.stats : FighterStats{};
}

bool ComboSystem::IsIdle(uint32_t handle) const
{
    auto it = m_impl->fighters.find(handle);
    return it == m_impl->fighters.end() || !it->second.awaitingInput;
}

void ComboSystem::SetOnComboBreak(ComboBreakCb cb)  { m_impl->onBreak = cb; }
void ComboSystem::SetOnFinisher(FinisherCb cb)       { m_impl->onFinisher = cb; }

} // namespace Runtime
