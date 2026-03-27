#include "Runtime/Dialogue/DialogueManager/DialogueManager.h"
#include <algorithm>
#include <fstream>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Runtime {

struct ConvCallbacks {
    uint32_t    convId{0};
    DialogueManager::NodeCb   onNodeEnter;
    DialogueManager::NodeCb   onNodeExit;
    DialogueManager::ChoiceCb onChoices;
    DialogueManager::FinishCb onFinish;
};

struct DialogueManager::Impl {
    std::unordered_map<std::string, ConversationDef> defs;
    std::vector<ActiveConversation>                  active;
    std::vector<ConvCallbacks>                       callbacks;
    std::unordered_set<std::string>                  flags;
    std::unordered_map<std::string,std::string>      variables;
    uint32_t nextConvId{1};

    ActiveConversation* FindConv(uint32_t id) {
        for (auto& c : active) if (c.convId==id) return &c;
        return nullptr;
    }
    ConvCallbacks* FindCb(uint32_t id) {
        for (auto& cb : callbacks) if (cb.convId==id) return &cb;
        return nullptr;
    }
    const ConversationDef* FindDef(const std::string& defId) const {
        auto it = defs.find(defId);
        return it!=defs.end() ? &it->second : nullptr;
    }

    void EnterNode(ActiveConversation& conv, const std::string& nodeId)
    {
        const auto* def  = FindDef(conv.defId);
        if (!def) return;
        auto nit = def->nodes.find(nodeId);
        if (nit==def->nodes.end()) { conv.finished=true; return; }

        // Exit previous
        if (!conv.currentNodeId.empty()) {
            auto pit = def->nodes.find(conv.currentNodeId);
            if (pit!=def->nodes.end()) {
                if (!pit->second.onExitFlag.empty()) flags.insert(pit->second.onExitFlag);
                auto* cb = FindCb(conv.convId);
                if (cb && cb->onNodeExit) cb->onNodeExit(pit->second);
            }
        }

        conv.currentNodeId = nodeId;
        const auto& node = nit->second;
        if (!node.onEnterFlag.empty()) flags.insert(node.onEnterFlag);
        conv.autoTimer = node.autoAdvanceDuration;

        auto* cb = FindCb(conv.convId);
        if (cb && cb->onNodeEnter) cb->onNodeEnter(node);

        // Build available choices
        std::vector<DialogueChoice> avail;
        for (auto c : node.choices) {
            if (!c.conditionFlag.empty() && !flags.count(c.conditionFlag)) continue;
            avail.push_back(c);
        }
        conv.waitingForChoice = !avail.empty();
        if (cb && !avail.empty() && cb->onChoices) cb->onChoices(avail);
    }
};

DialogueManager::DialogueManager()  : m_impl(new Impl) {}
DialogueManager::~DialogueManager() { Shutdown(); delete m_impl; }

void DialogueManager::Init()     {}
void DialogueManager::Shutdown() { m_impl->active.clear(); }

void DialogueManager::RegisterConversation(const ConversationDef& def)
{
    m_impl->defs[def.id] = def;
}

bool DialogueManager::LoadConversation(const std::string& path)
{
    std::ifstream f(path); return f.good();
}

bool DialogueManager::HasConversation(const std::string& defId) const
{
    return m_impl->defs.count(defId) > 0;
}

uint32_t DialogueManager::StartConversation(const std::string& defId,
                                              uint32_t actorA, uint32_t actorB)
{
    const auto* def = m_impl->FindDef(defId);
    if (!def) return 0;
    ActiveConversation conv;
    conv.convId      = m_impl->nextConvId++;
    conv.defId       = defId;
    conv.participantA= actorA;
    conv.participantB= actorB;
    m_impl->active.push_back(conv);
    m_impl->callbacks.push_back({conv.convId, {}, {}, {}, {}});
    m_impl->EnterNode(m_impl->active.back(), def->startNodeId);
    return conv.convId;
}

void DialogueManager::EndConversation(uint32_t convId)
{
    auto* conv = m_impl->FindConv(convId);
    if (!conv) return;
    conv->finished = true;
    auto* cb = m_impl->FindCb(convId);
    if (cb && cb->onFinish) cb->onFinish(convId);
}

bool DialogueManager::IsActive(uint32_t convId) const
{
    const auto* conv = m_impl->FindConv(convId);
    return conv && !conv->finished;
}

void DialogueManager::AdvanceLine(uint32_t convId)
{
    auto* conv = m_impl->FindConv(convId);
    if (!conv || conv->finished || conv->waitingForChoice) return;
    const auto* def  = m_impl->FindDef(conv->defId);
    if (!def) return;
    auto nit = def->nodes.find(conv->currentNodeId);
    if (nit==def->nodes.end()) return;
    const auto& node = nit->second;
    if (!node.nextNodeId.empty()) m_impl->EnterNode(*conv, node.nextNodeId);
    else EndConversation(convId);
}

void DialogueManager::SelectChoice(uint32_t convId, uint32_t choiceIndex)
{
    auto* conv = m_impl->FindConv(convId);
    if (!conv || conv->finished) return;
    const auto* def = m_impl->FindDef(conv->defId);
    if (!def) return;
    auto nit = def->nodes.find(conv->currentNodeId);
    if (nit==def->nodes.end()) return;

    std::vector<DialogueChoice> avail;
    for (auto& c : nit->second.choices) {
        if (!c.conditionFlag.empty() && !m_impl->flags.count(c.conditionFlag)) continue;
        avail.push_back(c);
    }
    if (choiceIndex >= (uint32_t)avail.size()) return;
    conv->waitingForChoice = false;
    const auto& choice = avail[choiceIndex];
    if (!choice.targetNodeId.empty()) m_impl->EnterNode(*conv, choice.targetNodeId);
    else EndConversation(convId);
}

std::vector<DialogueChoice> DialogueManager::GetChoices(uint32_t convId) const
{
    const auto* conv = m_impl->FindConv(convId);
    if (!conv) return {};
    const auto* def  = m_impl->FindDef(conv->defId);
    if (!def) return {};
    auto nit = def->nodes.find(conv->currentNodeId);
    if (nit==def->nodes.end()) return {};
    std::vector<DialogueChoice> out;
    for (auto& c : nit->second.choices) {
        if (!c.conditionFlag.empty() && !m_impl->flags.count(c.conditionFlag)) continue;
        out.push_back(c);
    }
    return out;
}

const DialogueNode* DialogueManager::GetCurrentNode(uint32_t convId) const
{
    const auto* conv = m_impl->FindConv(convId);
    if (!conv) return nullptr;
    const auto* def  = m_impl->FindDef(conv->defId);
    if (!def) return nullptr;
    auto nit = def->nodes.find(conv->currentNodeId);
    return nit!=def->nodes.end() ? &nit->second : nullptr;
}

void DialogueManager::SetFlag  (const std::string& f) { m_impl->flags.insert(f); }
void DialogueManager::ClearFlag(const std::string& f) { m_impl->flags.erase(f); }
bool DialogueManager::HasFlag  (const std::string& f) const { return m_impl->flags.count(f)>0; }

void DialogueManager::SetVariable(const std::string& key, const std::string& value)
{ m_impl->variables[key]=value; }

std::string DialogueManager::ResolveText(const std::string& text) const
{
    std::string out = text;
    for (auto& [k,v] : m_impl->variables) {
        std::string tok = "{" + k + "}";
        size_t pos;
        while ((pos=out.find(tok))!=std::string::npos) out.replace(pos, tok.size(), v);
    }
    return out;
}

void DialogueManager::SetOnNodeEnter(uint32_t convId, NodeCb cb)
{
    auto* c = m_impl->FindCb(convId); if(c) c->onNodeEnter=cb;
}
void DialogueManager::SetOnNodeExit(uint32_t convId, NodeCb cb)
{
    auto* c = m_impl->FindCb(convId); if(c) c->onNodeExit=cb;
}
void DialogueManager::SetOnChoicesAvailable(uint32_t convId, ChoiceCb cb)
{
    auto* c = m_impl->FindCb(convId); if(c) c->onChoices=cb;
}
void DialogueManager::SetOnConversationEnd(uint32_t convId, FinishCb cb)
{
    auto* c = m_impl->FindCb(convId); if(c) c->onFinish=cb;
}

void DialogueManager::Tick(float dt)
{
    for (auto& conv : m_impl->active) {
        if (conv.finished || conv.waitingForChoice) continue;
        const auto* def = m_impl->FindDef(conv.defId);
        if (!def) continue;
        auto nit = def->nodes.find(conv.currentNodeId);
        if (nit==def->nodes.end()) continue;
        if (nit->second.autoAdvanceDuration > 0.f) {
            conv.autoTimer -= dt;
            if (conv.autoTimer <= 0.f) AdvanceLine(conv.convId);
        }
    }
    // Cleanup finished
    auto& v = m_impl->active;
    v.erase(std::remove_if(v.begin(),v.end(),[](auto& c){ return c.finished; }), v.end());
}

} // namespace Runtime
