#include "Builder/Assembly/BuildQueue.h"
#include <algorithm>

namespace Builder {

float BuildOrder::Progress() const {
    if (totalTimeSeconds <= 0.0f) return 1.0f;
    return std::min(elapsedSeconds / totalTimeSeconds, 1.0f);
}

float BuildOrder::RemainingSeconds() const {
    return std::max(0.0f, totalTimeSeconds - elapsedSeconds);
}

uint32_t BuildQueue::AddOrder(BuildOrder order) {
    order.id = m_nextId++;
    m_active.push_back(order);
    return order.id;
}

void BuildQueue::RemoveOrder(uint32_t orderId) {
    auto it = std::find_if(m_active.begin(), m_active.end(),
        [orderId](const BuildOrder& o){ return o.id == orderId; });
    if (it != m_active.end()) m_active.erase(it);
}

void BuildQueue::PauseOrder(uint32_t orderId) {
    for (auto& o : m_active) if (o.id == orderId) { o.paused = true; return; }
}

void BuildQueue::ResumeOrder(uint32_t orderId) {
    for (auto& o : m_active) if (o.id == orderId) { o.paused = false; return; }
}

void BuildQueue::Tick(float deltaSeconds) {
    std::vector<BuildOrder> stillActive;
    for (auto& o : m_active) {
        if (!o.paused) o.elapsedSeconds += deltaSeconds;
        if (o.IsComplete()) m_completed.push_back(o);
        else stillActive.push_back(o);
    }
    m_active = std::move(stillActive);
}

const BuildOrder* BuildQueue::GetOrder(uint32_t orderId) const {
    for (const auto& o : m_active) if (o.id == orderId) return &o;
    return nullptr;
}

std::vector<BuildOrder> BuildQueue::GetOrdersByPriority() const {
    std::vector<BuildOrder> sorted = m_active;
    std::sort(sorted.begin(), sorted.end(),
        [](const BuildOrder& a, const BuildOrder& b){ return a.priority > b.priority; });
    return sorted;
}

float BuildQueue::TotalRemainingTime() const {
    float total = 0.0f;
    for (const auto& o : m_active) total += o.RemainingSeconds();
    return total;
}

} // namespace Builder
