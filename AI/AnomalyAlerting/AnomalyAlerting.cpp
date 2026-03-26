#include "AI/AnomalyAlerting/AnomalyAlerting.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace AI {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

// ── Per-metric rule ───────────────────────────────────────────────────────────

struct MetricRule {
    bool          hasThreshold{false};
    float         minVal{0.f}, maxVal{0.f};
    bool          hasZScore{false};
    float         maxZScore{3.0f};
    AlertSeverity severity{AlertSeverity::Warning};
    uint64_t      cooldownMs{5000};
    uint64_t      lastAlertMs{0};
    std::vector<float> history; ///< rolling sample history for z-score
};

struct AnomalyAlerting::Impl {
    std::unordered_map<std::string, MetricRule>   rules;
    std::vector<AnomalyAlert>                     activeAlerts;
    std::vector<AnomalyAlert>                     history;
    std::function<void(const AnomalyAlert&)>      onAlert;
    uint64_t                                      nextAlertId{1};

    void Fire(const std::string& metric, float value,
              float threshold, AlertSeverity sev, const std::string& msg) {
        uint64_t now = NowMs();
        auto& rule = rules[metric];
        if (now - rule.lastAlertMs < rule.cooldownMs) return; // rate limited
        rule.lastAlertMs = now;
        AnomalyAlert alert;
        alert.id          = nextAlertId++;
        alert.metric      = metric;
        alert.value       = value;
        alert.threshold   = threshold;
        alert.severity    = sev;
        alert.message     = msg;
        alert.timestampMs = now;
        activeAlerts.push_back(alert);
        history.push_back(alert);
        if (onAlert) onAlert(alert);
    }
};

AnomalyAlerting::AnomalyAlerting() : m_impl(new Impl()) {}
AnomalyAlerting::~AnomalyAlerting() { delete m_impl; }

void AnomalyAlerting::Init()     {}
void AnomalyAlerting::Shutdown() {}

void AnomalyAlerting::SetThreshold(const std::string& metric,
                                   float minVal, float maxVal,
                                   AlertSeverity severity) {
    auto& r = m_impl->rules[metric];
    r.hasThreshold = true;
    r.minVal       = minVal;
    r.maxVal       = maxVal;
    r.severity     = severity;
}

void AnomalyAlerting::SetZScoreLimit(const std::string& metric,
                                     float maxZScore,
                                     AlertSeverity severity) {
    auto& r = m_impl->rules[metric];
    r.hasZScore  = true;
    r.maxZScore  = maxZScore;
    r.severity   = severity;
}

void AnomalyAlerting::SetCooldownMs(const std::string& metric,
                                    uint64_t cooldownMs) {
    m_impl->rules[metric].cooldownMs = cooldownMs;
}

void AnomalyAlerting::Sample(const std::string& metric, float value) {
    auto& rule = m_impl->rules[metric];
    rule.history.push_back(value);
    if (rule.history.size() > 200) rule.history.erase(rule.history.begin());

    if (rule.hasThreshold) {
        if (value < rule.minVal) {
            std::ostringstream ss;
            ss << metric << " value " << value << " below min " << rule.minVal;
            m_impl->Fire(metric, value, rule.minVal, rule.severity, ss.str());
        } else if (value > rule.maxVal) {
            std::ostringstream ss;
            ss << metric << " value " << value << " above max " << rule.maxVal;
            m_impl->Fire(metric, value, rule.maxVal, rule.severity, ss.str());
        }
    }

    if (rule.hasZScore && rule.history.size() >= 5) {
        float mean = std::accumulate(rule.history.begin(), rule.history.end(), 0.f)
                     / (float)rule.history.size();
        float var  = 0.f;
        for (float v : rule.history) var += (v - mean) * (v - mean);
        var /= (float)rule.history.size();
        float stddev = std::sqrt(var);
        float z = stddev > 0.f ? std::abs(value - mean) / stddev : 0.f;
        if (z > rule.maxZScore) {
            std::ostringstream ss;
            ss << metric << " z-score " << z << " exceeds limit " << rule.maxZScore;
            m_impl->Fire(metric, value, rule.maxZScore, rule.severity, ss.str());
        }
    }
}

std::vector<AnomalyAlert> AnomalyAlerting::ActiveAlerts() const {
    std::vector<AnomalyAlert> out;
    for (auto& a : m_impl->activeAlerts)
        if (!a.acknowledged) out.push_back(a);
    return out;
}

std::vector<AnomalyAlert> AnomalyAlerting::AlertHistory() const {
    return m_impl->history;
}

void AnomalyAlerting::Acknowledge(uint64_t alertId) {
    for (auto& a : m_impl->activeAlerts)
        if (a.id == alertId) { a.acknowledged = true; break; }
}

void AnomalyAlerting::AcknowledgeAll() {
    for (auto& a : m_impl->activeAlerts) a.acknowledged = true;
}

void AnomalyAlerting::ClearHistory() {
    m_impl->activeAlerts.clear();
    m_impl->history.clear();
}

void AnomalyAlerting::OnAlert(std::function<void(const AnomalyAlert&)> cb) {
    m_impl->onAlert = std::move(cb);
}

} // namespace AI
