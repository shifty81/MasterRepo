#include "PCG/Narrative/NarrativeEngine.h"
#include <algorithm>
#include <numeric>
#include <random>
#include <sstream>

namespace PCG {

void NarrativeEngine::Init() {
    m_ctx = NarrativeContext{};
    m_templates.clear();
    m_events.clear();
    m_arcs.clear();
    m_nextEventId = 1;
    m_nextArcId   = 1;
}

// ── Context management ────────────────────────────────────────────────────────

void  NarrativeEngine::SetContext(const std::string& key, float value)       { m_ctx.numbers[key] = value; }
void  NarrativeEngine::SetContext(const std::string& key, const std::string& v) { m_ctx.strings[key] = v; }
void  NarrativeEngine::SetFlag(const std::string& flag)                      { m_ctx.flags.insert(flag); }
void  NarrativeEngine::ClearFlag(const std::string& flag)                    { m_ctx.flags.erase(flag); }
bool  NarrativeEngine::HasFlag(const std::string& flag) const                { return m_ctx.flags.count(flag) > 0; }
float NarrativeEngine::GetNumber(const std::string& key, float def) const {
    auto it = m_ctx.numbers.find(key);
    return it != m_ctx.numbers.end() ? it->second : def;
}
const NarrativeContext& NarrativeEngine::GetContext() const { return m_ctx; }

// ── Dialogue ──────────────────────────────────────────────────────────────────

void NarrativeEngine::AddDialogueTemplate(const DialogueTemplate& tmpl) {
    m_templates[tmpl.id] = tmpl;
}

std::string NarrativeEngine::GenerateLine(const std::string& templateId) const {
    return GenerateLine(templateId, NarrativeContext{});
}

std::string NarrativeEngine::GenerateLine(const std::string& templateId,
                                           const NarrativeContext& overrides) const {
    auto it = m_templates.find(templateId);
    if (it == m_templates.end()) return "";

    // Merge overrides into a combined context
    NarrativeContext combined = m_ctx;
    for (const auto& [k, v] : overrides.numbers)  combined.numbers[k] = v;
    for (const auto& [k, v] : overrides.strings)  combined.strings[k] = v;
    for (const auto& f : overrides.flags)          combined.flags.insert(f);

    // Collect eligible variants weighted by condition + weight
    std::vector<size_t> eligible;
    std::vector<float>  weights;
    const auto& variants = it->second.variants;
    for (size_t i = 0; i < variants.size(); ++i) {
        const auto& v = variants[i];
        if (!v.condition || v.condition(combined)) {
            eligible.push_back(i);
            weights.push_back(v.weight);
        }
    }
    if (eligible.empty()) return "";

    // Weighted random selection (deterministic seed for reproducibility)
    static std::mt19937 rng(0xDEADBEEF);
    std::discrete_distribution<size_t> dist(weights.begin(), weights.end());
    size_t chosen = eligible[dist(rng)];
    return Substitute(variants[chosen].text, combined);
}

// ── Events ────────────────────────────────────────────────────────────────────

uint32_t NarrativeEngine::AddEvent(NarrativeEvent evt) {
    evt.id = m_nextEventId++;
    uint32_t id = evt.id;
    m_events[id] = std::move(evt);
    return id;
}

void NarrativeEngine::FireEvent(uint32_t eventId) {
    auto it = m_events.find(eventId);
    if (it == m_events.end()) return;
    NarrativeEvent& evt = it->second;
    if (evt.fired && !evt.repeatable) return;
    evt.fired = true;
    for (const auto& f : evt.setsFlags)  m_ctx.flags.insert(f);
    for (const auto& f : evt.clearFlags) m_ctx.flags.erase(f);
    if (m_onEventFired) m_onEventFired(evt);
}

void NarrativeEngine::EvaluateEvents() {
    for (auto& [id, evt] : m_events) {
        if (evt.fired && !evt.repeatable) continue;
        if (evt.trigger && evt.trigger(m_ctx))
            FireEvent(id);
    }
}

// ── Story arcs ────────────────────────────────────────────────────────────────

uint32_t NarrativeEngine::AddStoryArc(const std::string& name,
                                       const std::vector<uint32_t>& eventIds) {
    StoryArc arc;
    arc.id       = m_nextArcId++;
    arc.name     = name;
    arc.eventIds = eventIds;
    uint32_t id  = arc.id;
    m_arcs[id]   = std::move(arc);
    return id;
}

StoryArc* NarrativeEngine::GetStoryArc(uint32_t arcId) {
    auto it = m_arcs.find(arcId);
    return it != m_arcs.end() ? &it->second : nullptr;
}

void NarrativeEngine::AdvanceArc(uint32_t arcId) {
    auto it = m_arcs.find(arcId);
    if (it == m_arcs.end() || it->second.complete) return;
    StoryArc& arc = it->second;
    if (arc.progress >= arc.eventIds.size()) { arc.complete = true; return; }
    FireEvent(arc.eventIds[arc.progress]);
    ++arc.progress;
    if (arc.progress >= arc.eventIds.size()) {
        arc.complete = true;
        if (m_onArcCompleted) m_onArcCompleted(arc);
    }
}

bool NarrativeEngine::IsArcComplete(uint32_t arcId) const {
    auto it = m_arcs.find(arcId);
    return it != m_arcs.end() && it->second.complete;
}

// ── Tick ─────────────────────────────────────────────────────────────────────

void NarrativeEngine::Tick(float /*dt*/) {
    EvaluateEvents();
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void NarrativeEngine::SetEventFiredCallback(NarrativeEventFiredFn fn) { m_onEventFired  = std::move(fn); }
void NarrativeEngine::SetArcCompletedCallback(ArcCompletedFn fn)      { m_onArcCompleted = std::move(fn); }

// ── Stats ─────────────────────────────────────────────────────────────────────

size_t NarrativeEngine::DialogueTemplateCount() const { return m_templates.size(); }
size_t NarrativeEngine::EventCount()            const { return m_events.size(); }
size_t NarrativeEngine::ArcCount()              const { return m_arcs.size(); }

// ── Private helpers ───────────────────────────────────────────────────────────

std::string NarrativeEngine::Substitute(const std::string& text,
                                         const NarrativeContext& ctx) const {
    std::string result;
    result.reserve(text.size());
    size_t i = 0;
    while (i < text.size()) {
        if (text[i] == '{') {
            size_t end = text.find('}', i + 1);
            if (end != std::string::npos) {
                std::string key = text.substr(i + 1, end - i - 1);
                // Try strings first
                auto sit = ctx.strings.find(key);
                if (sit != ctx.strings.end()) {
                    result += sit->second;
                } else {
                    auto nit = ctx.numbers.find(key);
                    if (nit != ctx.numbers.end()) {
                        std::ostringstream ss;
                        ss << nit->second;
                        result += ss.str();
                    } else {
                        result += '{'; result += key; result += '}';
                    }
                }
                i = end + 1;
                continue;
            }
        }
        result += text[i++];
    }
    return result;
}

} // namespace PCG
