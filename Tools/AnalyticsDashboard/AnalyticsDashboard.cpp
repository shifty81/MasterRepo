#include "Tools/AnalyticsDashboard/AnalyticsDashboard.h"
#include <algorithm>
#include <ctime>
#include <numeric>
#include <fstream>
#include <sstream>

namespace Tools {

// ── MetricSeries helpers ───────────────────────────────────────

double MetricSeries::Min() const {
    if (samples.empty()) return 0.0;
    return std::min_element(samples.begin(), samples.end(),
           [](const MetricSample& a, const MetricSample& b){ return a.value < b.value; })
           ->value;
}
double MetricSeries::Max() const {
    if (samples.empty()) return 0.0;
    return std::max_element(samples.begin(), samples.end(),
           [](const MetricSample& a, const MetricSample& b){ return a.value < b.value; })
           ->value;
}
double MetricSeries::Mean() const {
    if (samples.empty()) return 0.0;
    double sum = std::accumulate(samples.begin(), samples.end(), 0.0,
                 [](double acc, const MetricSample& s){ return acc + s.value; });
    return sum / static_cast<double>(samples.size());
}
double MetricSeries::Last() const {
    return samples.empty() ? 0.0 : samples.back().value;
}

// ── Singleton ──────────────────────────────────────────────────

AnalyticsDashboard& AnalyticsDashboard::Get() {
    static AnalyticsDashboard instance;
    return instance;
}

// ── Recording ─────────────────────────────────────────────────

void AnalyticsDashboard::Record(const std::string& name, double value,
                                 const std::string& tag) {
    auto& series = m_series[name];
    if (series.name.empty()) series.name = name;
    MetricSample sample{value, CurrentTimestamp(), tag};
    series.samples.push_back(sample);
    CheckAlerts(name, value);
}

void AnalyticsDashboard::StartTimer(const std::string& name) {
    m_timers[name] = std::chrono::steady_clock::now();
}

void AnalyticsDashboard::StopTimer(const std::string& name) {
    auto it = m_timers.find(name);
    if (it == m_timers.end()) return;
    auto elapsed = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - it->second).count();
    Record(name, elapsed, "ms");
    m_timers.erase(it);
}

void AnalyticsDashboard::IncrementCounter(const std::string& name, double by) {
    auto& series = m_series[name];
    double current = series.samples.empty() ? 0.0 : series.samples.back().value;
    Record(name, current + by);
}

void AnalyticsDashboard::RecordFPS(double fps)            { Record("fps", fps); }
void AnalyticsDashboard::RecordFrameTime(double ms)       { Record("frame_time_ms", ms); }
void AnalyticsDashboard::RecordMemoryUsage(double bytes)  { Record("memory_bytes", bytes); }
void AnalyticsDashboard::RecordDrawCalls(double count)    { Record("draw_calls", count); }
void AnalyticsDashboard::RecordTriangleCount(double count){ Record("triangle_count", count); }

// ── Access ─────────────────────────────────────────────────────

const MetricSeries* AnalyticsDashboard::GetSeries(const std::string& name) const {
    auto it = m_series.find(name);
    return (it != m_series.end()) ? &it->second : nullptr;
}

std::vector<std::string> AnalyticsDashboard::ListMetrics() const {
    std::vector<std::string> names;
    names.reserve(m_series.size());
    for (auto& [name, _] : m_series) names.push_back(name);
    std::sort(names.begin(), names.end());
    return names;
}

// ── Widgets ────────────────────────────────────────────────────

void AnalyticsDashboard::AddWidget(const DashboardWidget& widget) {
    m_widgets.push_back(widget);
}

void AnalyticsDashboard::RemoveWidget(const std::string& id) {
    m_widgets.erase(std::remove_if(m_widgets.begin(), m_widgets.end(),
        [&](const DashboardWidget& w){ return w.id == id; }), m_widgets.end());
}

// ── Export ─────────────────────────────────────────────────────

bool AnalyticsDashboard::ExportToCSV(const std::string& filePath) const {
    std::ofstream f(filePath);
    if (!f.is_open()) return false;
    f << "metric,timestamp,value,tag\n";
    for (auto& [name, series] : m_series) {
        for (auto& s : series.samples) {
            f << name << "," << s.timestamp << "," << s.value << "," << s.tag << "\n";
        }
    }
    return true;
}

bool AnalyticsDashboard::ExportToJSON(const std::string& filePath) const {
    std::ofstream f(filePath);
    if (!f.is_open()) return false;
    f << "{\n  \"metrics\": {\n";
    bool firstMetric = true;
    for (auto& [name, series] : m_series) {
        if (!firstMetric) f << ",\n";
        firstMetric = false;
        f << "    \"" << name << "\": [";
        bool firstSample = true;
        for (auto& s : series.samples) {
            if (!firstSample) f << ",";
            firstSample = false;
            f << s.value;
        }
        f << "]";
    }
    f << "\n  }\n}\n";
    return true;
}

// ── Alerts ─────────────────────────────────────────────────────

void AnalyticsDashboard::SetAlert(const std::string& metric, double threshold,
                                   bool alertAbove, AlertCallback cb) {
    m_alerts[metric] = {threshold, alertAbove, std::move(cb)};
}

void AnalyticsDashboard::CheckAlerts(const std::string& name, double value) {
    auto it = m_alerts.find(name);
    if (it == m_alerts.end()) return;
    const Alert& a = it->second;
    bool triggered = a.alertAbove ? (value > a.threshold) : (value < a.threshold);
    if (triggered && a.callback) a.callback(name, value);
}

// ── Render stub ────────────────────────────────────────────────

void AnalyticsDashboard::Render() {
    // Dispatches to platform UI layer; rendered by UI::GUISystem in production
}

void AnalyticsDashboard::Reset() {
    m_series.clear();
    m_timers.clear();
}

// ── Private ────────────────────────────────────────────────────

std::string AnalyticsDashboard::CurrentTimestamp() const {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
    return buf;
}

} // namespace Tools
