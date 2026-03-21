#include "AI/MetricsDashboard/MetricsDashboard.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <sstream>

namespace fs = std::filesystem;
namespace AI {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/* static */
double MetricsDashboard::NowSecs()
{
    return std::chrono::duration<double>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

MetricsDashboard::MetricsDashboard(MetricsDashboardConfig cfg)
    : m_cfg(std::move(cfg))
{}

MetricsDashboard::~MetricsDashboard() {}

// ── Registration ──────────────────────────────────────────────────────────────

void MetricsDashboard::RegisterMetric(MetricStream stream)
{
    if (stream.maxSamples <= 0)
        stream.maxSamples = m_cfg.defaultMaxSamples;
    m_streams[stream.id] = std::move(stream);
}

void MetricsDashboard::UnregisterMetric(const std::string& id)
{
    m_streams.erase(id);
}

bool MetricsDashboard::HasMetric(const std::string& id) const
{
    return m_streams.count(id) > 0;
}

// ── Recording ─────────────────────────────────────────────────────────────────

MetricStream* MetricsDashboard::FindStream(const std::string& id)
{
    auto it = m_streams.find(id);
    return (it != m_streams.end()) ? &it->second : nullptr;
}

const MetricStream* MetricsDashboard::FindStream(const std::string& id) const
{
    auto it = m_streams.find(id);
    return (it != m_streams.end()) ? &it->second : nullptr;
}

void MetricsDashboard::Record(const std::string& id, float value,
                               double timestamp)
{
    auto* s = FindStream(id);
    if (!s) return;

    MetricSample sample;
    sample.timestamp = (timestamp > 0.0) ? timestamp : NowSecs();
    sample.value     = value;

    s->samples.push_back(sample);
    while ((int)s->samples.size() > s->maxSamples)
        s->samples.erase(s->samples.begin());
}

void MetricsDashboard::RecordMany(
    const std::unordered_map<std::string,float>& values,
    double timestamp)
{
    double ts = (timestamp > 0.0) ? timestamp : NowSecs();
    for (const auto& [id, v] : values)
        Record(id, v, ts);
}

// ── Queries ───────────────────────────────────────────────────────────────────

float MetricsDashboard::Latest(const std::string& id) const
{
    const auto* s = FindStream(id);
    if (!s || s->samples.empty()) return 0.f;
    return s->samples.back().value;
}

static std::vector<float> LastNValues(const MetricStream& s, int n)
{
    int count = (n <= 0 || n > (int)s.samples.size())
                ? (int)s.samples.size() : n;
    std::vector<float> vals;
    vals.reserve(count);
    for (int i = (int)s.samples.size() - count;
         i < (int)s.samples.size(); ++i)
        vals.push_back(s.samples[i].value);
    return vals;
}

float MetricsDashboard::Average(const std::string& id, int lastN) const
{
    const auto* s = FindStream(id);
    if (!s || s->samples.empty()) return 0.f;
    auto vals = LastNValues(*s, lastN);
    return std::accumulate(vals.begin(), vals.end(), 0.f) / (float)vals.size();
}

float MetricsDashboard::Min(const std::string& id, int lastN) const
{
    const auto* s = FindStream(id);
    if (!s || s->samples.empty()) return 0.f;
    auto vals = LastNValues(*s, lastN);
    return *std::min_element(vals.begin(), vals.end());
}

float MetricsDashboard::Max(const std::string& id, int lastN) const
{
    const auto* s = FindStream(id);
    if (!s || s->samples.empty()) return 0.f;
    auto vals = LastNValues(*s, lastN);
    return *std::max_element(vals.begin(), vals.end());
}

float MetricsDashboard::Percentile(const std::string& id, float p,
                                    int lastN) const
{
    const auto* s = FindStream(id);
    if (!s || s->samples.empty()) return 0.f;
    auto vals = LastNValues(*s, lastN);
    std::sort(vals.begin(), vals.end());
    int idx = std::max(0, (int)(p * (float)(vals.size() - 1)));
    return vals[idx];
}

std::vector<MetricSample> MetricsDashboard::GetSamples(
    const std::string& id, int lastN) const
{
    const auto* s = FindStream(id);
    if (!s) return {};
    if (lastN <= 0 || lastN >= (int)s->samples.size()) return s->samples;
    return std::vector<MetricSample>(
        s->samples.end() - lastN, s->samples.end());
}

std::vector<std::string> MetricsDashboard::GetMetricIds(
    MetricCategory cat) const
{
    std::vector<std::string> out;
    for (const auto& [id, s] : m_streams)
        if (s.category == cat) out.push_back(id);
    return out;
}

// ── Alerting ──────────────────────────────────────────────────────────────────

void MetricsDashboard::AddAlert(MetricAlert alert)
{
    m_alerts.push_back(std::move(alert));
}

void MetricsDashboard::RemoveAlert(const std::string& metricId)
{
    m_alerts.erase(
        std::remove_if(m_alerts.begin(), m_alerts.end(),
                       [&](const MetricAlert& a){
                           return a.metricId == metricId;
                       }),
        m_alerts.end());
}

std::vector<FiredAlert> MetricsDashboard::CheckAlerts() const
{
    std::vector<FiredAlert> fired;
    double now = NowSecs();
    for (const auto& a : m_alerts) {
        float v = Latest(a.metricId);
        bool triggered = a.aboveThreshold ? (v > a.threshold) : (v < a.threshold);
        if (triggered) {
            FiredAlert fa;
            fa.alert     = a;
            fa.value     = v;
            fa.timestamp = now;
            fired.push_back(fa);
        }
    }
    return fired;
}

void MetricsDashboard::SetAlertCallback(AlertFn fn) { m_alertCb = std::move(fn); }

// ── Tick ──────────────────────────────────────────────────────────────────────

void MetricsDashboard::Tick(double now)
{
    if (now <= 0.0) now = NowSecs();

    if (m_cfg.enableAlerts && m_alertCb) {
        auto fired = CheckAlerts();
        for (const auto& fa : fired)
            m_alertCb(fa);
    }

    if (!m_cfg.csvFlushPath.empty() && m_cfg.csvFlushIntervalSec > 0) {
        if (now - m_lastFlush >= m_cfg.csvFlushIntervalSec) {
            FlushCSV();
            m_lastFlush = now;
        }
    }
}

// ── Reporting ─────────────────────────────────────────────────────────────────

std::string MetricsDashboard::BuildMarkdownReport(MetricCategory cat) const
{
    std::ostringstream oss;
    const char* catName =
        (cat == MetricCategory::PCG)       ? "PCG"       :
        (cat == MetricCategory::Rendering) ? "Rendering" :
        (cat == MetricCategory::Gameplay)  ? "Gameplay"  :
        (cat == MetricCategory::AIAgent)   ? "AI Agent"  : "Custom";

    oss << "## " << catName << " Metrics\n\n"
        << "| Metric | Latest | Avg | Min | Max |\n"
        << "|--------|--------|-----|-----|-----|\n";

    for (const auto& [id, s] : m_streams) {
        if (s.category != cat) continue;
        oss << "| " << s.displayName << " (" << s.unit << ") "
            << "| " << Latest(id)   << " "
            << "| " << Average(id)  << " "
            << "| " << Min(id)      << " "
            << "| " << Max(id)      << " |\n";
    }
    return oss.str();
}

std::string MetricsDashboard::BuildFullReport() const
{
    std::ostringstream oss;
    oss << "# Atlas Engine Metrics Dashboard\n\n";
    for (auto cat : {MetricCategory::PCG, MetricCategory::Rendering,
                     MetricCategory::Gameplay, MetricCategory::AIAgent,
                     MetricCategory::Custom})
        oss << BuildMarkdownReport(cat);
    return oss.str();
}

bool MetricsDashboard::FlushCSV() const
{
    if (m_cfg.csvFlushPath.empty()) return false;
    try {
        fs::create_directories(fs::path(m_cfg.csvFlushPath).parent_path());
        std::ofstream f(m_cfg.csvFlushPath);
        if (!f) return false;
        f << "id,timestamp,value\n";
        for (const auto& [id, s] : m_streams)
            for (const auto& sample : s.samples)
                f << id << "," << sample.timestamp << "," << sample.value << "\n";
        return true;
    } catch (...) {
        return false;
    }
}

size_t MetricsDashboard::MetricCount() const { return m_streams.size(); }

size_t MetricsDashboard::TotalSamples() const
{
    size_t n = 0;
    for (const auto& [id, s] : m_streams)
        n += s.samples.size();
    return n;
}

} // namespace AI
