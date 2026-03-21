#include "Tools/MetricsReporting/MetricsReporter.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
namespace Tools {

// ─────────────────────────────────────────────────────────────────────────────
// ReportRenderer
// ─────────────────────────────────────────────────────────────────────────────

/* static */
std::string ReportRenderer::RenderMarkdown(
    const std::string& title,
    const std::vector<MetricSnapshot>& snapshots)
{
    std::ostringstream oss;
    oss << "# " << title << "\n\n";
    if (snapshots.empty()) { oss << "*No data.*\n"; return oss.str(); }

    // Collect all metric names across snapshots
    std::vector<std::string> names;
    for (const auto& snap : snapshots)
        for (const auto& [k,v] : snap.values) {
            if (std::find(names.begin(), names.end(), k) == names.end())
                names.push_back(k);
        }
    std::sort(names.begin(), names.end());

    // Table: one row per metric, last/min/max
    oss << "| Metric | Latest | Min | Max |\n|--------|--------|-----|-----|\n";
    for (const auto& name : names) {
        float latest = 0.f, mn = 1e9f, mx = -1e9f;
        for (const auto& snap : snapshots) {
            auto it = snap.values.find(name);
            if (it == snap.values.end()) continue;
            float v = it->second;
            latest = v;
            mn = std::min(mn, v);
            mx = std::max(mx, v);
        }
        oss << "| " << name << " | " << latest << " | " << mn << " | " << mx << " |\n";
    }
    return oss.str();
}

/* static */
std::string ReportRenderer::RenderJSON(
    const std::string& title,
    const std::vector<MetricSnapshot>& snapshots)
{
    std::ostringstream oss;
    oss << "{\"title\":\"" << title << "\",\"snapshots\":[\n";
    bool first = true;
    for (const auto& snap : snapshots) {
        if (!first) oss << ",\n";
        first = false;
        oss << "{\"ts\":" << snap.timestamp << ",\"source\":\"" << snap.source << "\",\"values\":{";
        bool fv = true;
        for (const auto& [k,v] : snap.values) {
            if (!fv) oss << ",";
            fv = false;
            oss << "\"" << k << "\":" << v;
        }
        oss << "}}";
    }
    oss << "\n]}";
    return oss.str();
}

/* static */
std::string ReportRenderer::RenderCSV(const std::vector<MetricSnapshot>& snapshots)
{
    if (snapshots.empty()) return "timestamp,source\n";

    // Collect column names
    std::vector<std::string> cols;
    for (const auto& snap : snapshots)
        for (const auto& [k,v] : snap.values)
            if (std::find(cols.begin(), cols.end(), k) == cols.end())
                cols.push_back(k);
    std::sort(cols.begin(), cols.end());

    std::ostringstream oss;
    oss << "timestamp,source";
    for (const auto& c : cols) oss << "," << c;
    oss << "\n";

    for (const auto& snap : snapshots) {
        oss << snap.timestamp << "," << snap.source;
        for (const auto& c : cols) {
            oss << ",";
            auto it = snap.values.find(c);
            if (it != snap.values.end()) oss << it->second;
        }
        oss << "\n";
    }
    return oss.str();
}

/* static */
std::string ReportRenderer::RenderPrometheus(const MetricSnapshot& latest)
{
    std::ostringstream oss;
    for (const auto& [k,v] : latest.values)
        oss << "atlas_" << k << " " << v << "\n";
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// MetricsReporter
// ─────────────────────────────────────────────────────────────────────────────

/* static */
double MetricsReporter::NowSecs()
{
    return std::chrono::duration<double>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

MetricsReporter::MetricsReporter(MetricsReporterConfig cfg)
    : m_cfg(std::move(cfg))
{}

MetricsReporter::~MetricsReporter() {}

// ── Schedule management ───────────────────────────────────────────────────────

void MetricsReporter::AddSchedule(ReportSchedule schedule)
{
    auto it = std::find_if(m_schedules.begin(), m_schedules.end(),
                           [&](const ReportSchedule& s){ return s.name == schedule.name; });
    if (it != m_schedules.end()) *it = std::move(schedule);
    else                         m_schedules.push_back(std::move(schedule));
}

void MetricsReporter::RemoveSchedule(const std::string& name)
{
    m_schedules.erase(
        std::remove_if(m_schedules.begin(), m_schedules.end(),
                       [&](const ReportSchedule& s){ return s.name == name; }),
        m_schedules.end());
}

void MetricsReporter::SetScheduleEnabled(const std::string& name, bool enabled)
{
    for (auto& s : m_schedules)
        if (s.name == name) s.enabled = enabled;
}

// ── Ingestion ─────────────────────────────────────────────────────────────────

void MetricsReporter::Feed(const MetricSnapshot& snap)
{
    m_snapshots.push_back(snap);
    while ((int)m_snapshots.size() > m_cfg.maxHistory)
        m_snapshots.erase(m_snapshots.begin());
}

void MetricsReporter::FeedValues(
    const std::string& source,
    const std::unordered_map<std::string,float>& values,
    double timestamp)
{
    MetricSnapshot snap;
    snap.source    = source;
    snap.timestamp = (timestamp > 0.0) ? timestamp : NowSecs();
    snap.values    = values;
    Feed(snap);
}

// ── Tick ──────────────────────────────────────────────────────────────────────

void MetricsReporter::Tick(double now)
{
    if (now <= 0.0) now = NowSecs();
    for (const auto& sched : m_schedules) {
        if (!sched.enabled || sched.intervalSec <= 0.f) continue;
        auto it = m_lastFired.find(sched.name);
        double last = (it != m_lastFired.end()) ? it->second : 0.0;
        if (now - last >= sched.intervalSec)
            FireSchedule(sched, now);
    }
}

void MetricsReporter::FireSchedule(const ReportSchedule& sched, double now)
{
    auto doc = GenerateReport(sched.name, sched.format);
    DeliverReport(doc);
    m_history[sched.name].push_back(doc);
    m_lastFired[sched.name] = now;
    ++m_reportsFired;
}

// ── Report generation ────────────────────────────────────────────────────────

ReportDocument MetricsReporter::GenerateReport(
    const std::string& scheduleName) const
{
    for (const auto& s : m_schedules)
        if (s.name == scheduleName)
            return GenerateReport(scheduleName, s.format);
    return GenerateReport(scheduleName, ReportFormat::Markdown);
}

ReportDocument MetricsReporter::GenerateReport(
    const std::string& title, ReportFormat format) const
{
    ReportDocument doc;
    doc.scheduleName = title;
    doc.format       = format;

    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    doc.timestamp = buf;

    switch (format) {
        case ReportFormat::Markdown:
            doc.content = ReportRenderer::RenderMarkdown(title, m_snapshots);
            break;
        case ReportFormat::JSON:
            doc.content = ReportRenderer::RenderJSON(title, m_snapshots);
            break;
        case ReportFormat::CSV:
            doc.content = ReportRenderer::RenderCSV(m_snapshots);
            break;
        case ReportFormat::Prometheus:
            if (!m_snapshots.empty())
                doc.content = ReportRenderer::RenderPrometheus(m_snapshots.back());
            break;
    }
    return doc;
}

bool MetricsReporter::DeliverReport(ReportDocument& doc) const
{
    bool ok = true;

    // Write to file
    for (const auto& s : m_schedules) {
        if (s.name == doc.scheduleName && !s.outputPath.empty()) {
            ok = WriteToFile(doc);
            break;
        }
    }
    if (m_cfg.printToConsole && !doc.content.empty())
        ; // std::cout << doc.content << "\n"; // optional

    if (m_deliveryCb) m_deliveryCb(doc);
    doc.delivered = ok;
    return ok;
}

bool MetricsReporter::WriteToFile(const ReportDocument& doc) const
{
    // Find output path for this schedule
    std::string path;
    for (const auto& s : m_schedules)
        if (s.name == doc.scheduleName) { path = s.outputPath; break; }

    if (path.empty()) {
        // Default: Logs/Reports/<name>.md
        std::string ext = (doc.format == ReportFormat::JSON)  ? ".json" :
                          (doc.format == ReportFormat::CSV)   ? ".csv"  : ".md";
        path = m_cfg.logDir + "/" + doc.scheduleName + ext;
    }

    if (!m_cfg.repoRoot.empty() && path[0] != '/')
        path = m_cfg.repoRoot + "/" + path;

    try {
        fs::create_directories(fs::path(path).parent_path());
        std::ofstream f(path);
        if (!f) return false;
        f << doc.content;
        return true;
    } catch (...) {
        return false;
    }
}

bool MetricsReporter::PostWebhook(const ReportDocument& /*doc*/) const
{
    // Offline stub — would POST via libcurl if available
    return false;
}

// ── History ───────────────────────────────────────────────────────────────────

std::vector<ReportDocument> MetricsReporter::GetHistory(
    const std::string& scheduleName, int lastN) const
{
    auto it = m_history.find(scheduleName);
    if (it == m_history.end()) return {};
    const auto& h = it->second;
    if (lastN <= 0 || lastN >= (int)h.size()) return h;
    return std::vector<ReportDocument>(h.end() - lastN, h.end());
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void MetricsReporter::SetDeliveryCallback(DeliveryFn fn)
{
    m_deliveryCb = std::move(fn);
}

// ── Stats ─────────────────────────────────────────────────────────────────────

size_t MetricsReporter::SnapshotCount() const { return m_snapshots.size(); }
size_t MetricsReporter::ScheduleCount() const { return m_schedules.size(); }
size_t MetricsReporter::ReportsFired()  const { return m_reportsFired; }

} // namespace Tools
