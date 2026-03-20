#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>

namespace Tools {

// ──────────────────────────────────────────────────────────────
// Metric value types
// ──────────────────────────────────────────────────────────────

enum class MetricType { Counter, Gauge, Histogram, Timer };

struct MetricSample {
    double      value     = 0.0;
    std::string timestamp;
    std::string tag;
};

struct MetricSeries {
    std::string              name;
    MetricType               type  = MetricType::Gauge;
    std::string              unit;
    std::vector<MetricSample> samples;
    double Min()  const;
    double Max()  const;
    double Mean() const;
    double Last() const;
};

// ──────────────────────────────────────────────────────────────
// Dashboard widget (display config)
// ──────────────────────────────────────────────────────────────

enum class WidgetKind { LineChart, BarChart, Gauge, Table, TextLog };

struct DashboardWidget {
    std::string              id;
    std::string              title;
    WidgetKind               kind        = WidgetKind::LineChart;
    std::vector<std::string> metricNames;
    float                    refreshHz   = 1.f;
    uint32_t                 maxSamples  = 200;
};

// ──────────────────────────────────────────────────────────────
// AnalyticsDashboard — performance metrics collector & dashboard
// ──────────────────────────────────────────────────────────────

class AnalyticsDashboard {
public:
    // Metric recording
    void Record(const std::string& name, double value,
                const std::string& tag = "");
    void StartTimer(const std::string& name);
    void StopTimer(const std::string& name);
    void IncrementCounter(const std::string& name, double by = 1.0);

    // Built-in engine metrics helpers
    void RecordFPS(double fps);
    void RecordFrameTime(double ms);
    void RecordMemoryUsage(double bytes);
    void RecordDrawCalls(double count);
    void RecordTriangleCount(double count);

    // Series access
    const MetricSeries*       GetSeries(const std::string& name) const;
    std::vector<std::string>  ListMetrics() const;

    // Dashboard layout
    void AddWidget(const DashboardWidget& widget);
    void RemoveWidget(const std::string& id);
    const std::vector<DashboardWidget>& Widgets() const { return m_widgets; }

    // Export
    bool ExportToCSV(const std::string& filePath) const;
    bool ExportToJSON(const std::string& filePath) const;

    // Alerts: notify when a metric crosses a threshold
    using AlertCallback = std::function<void(const std::string& metric, double value)>;
    void SetAlert(const std::string& metric, double threshold,
                  bool alertAbove, AlertCallback cb);

    // Render (draws all active widgets into the UI layer)
    void Render();

    // Reset
    void Reset();

    // Singleton
    static AnalyticsDashboard& Get();

private:
    struct Alert {
        double        threshold;
        bool          alertAbove;
        AlertCallback callback;
    };

    std::string CurrentTimestamp() const;
    void        CheckAlerts(const std::string& name, double value);

    std::unordered_map<std::string, MetricSeries>    m_series;
    std::unordered_map<std::string,
        std::chrono::steady_clock::time_point>       m_timers;
    std::unordered_map<std::string, Alert>           m_alerts;
    std::vector<DashboardWidget>                     m_widgets;
};

} // namespace Tools
