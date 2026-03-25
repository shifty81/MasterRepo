#include "Tools/GUI/panels/PerfProfilerPanel.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

#include "Tools/Profiler/PerformanceProfiler.h"
#include "UI/Widgets/Widget.h"

namespace Tools {

struct PerfProfilerPanel::Impl {
    PerformanceProfiler* profiler{nullptr};
};

PerfProfilerPanel::PerfProfilerPanel() : m_impl(new Impl{}) {}
PerfProfilerPanel::~PerfProfilerPanel() { delete m_impl; }

void PerfProfilerPanel::OnAttach()  {}
void PerfProfilerPanel::OnDetach()  {}
void PerfProfilerPanel::OnUpdate(float /*dt*/) {}

void PerfProfilerPanel::SetProfiler(PerformanceProfiler* profiler) {
    m_impl->profiler = profiler;
}

void PerfProfilerPanel::OnDraw(const UI::UIStyle& style) {
    // [RENDER: panel background = style.panelBackground]

    UI::Label title("Performance Profiler");
    title.SetBounds({0, 0, 400, 24});
    // [RENDER: title, colour = style.textHighlight, fontSizeLarge]
    title.OnEvent(UI::WidgetEvent::None, 0, 0);

    if (!m_impl->profiler) {
        UI::Label noData("No PerformanceProfiler attached.");
        noData.SetBounds({0, 30, 400, 20});
        noData.OnEvent(UI::WidgetEvent::None, 0, 30);
        return;
    }

    PerformanceProfiler& prof = *m_impl->profiler;
    float rowY = 30.f;

    // ── Average frame time ───────────────────────────────────────────────────
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(3)
           << "Avg frame time: " << prof.AverageFrameTimeUs() << " us";
        UI::Label lbl(ss.str());
        lbl.SetBounds({0, rowY, 500, 20});
        // [RENDER: lbl, colour = style.textPrimary]
        lbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 28;
    }

    // ── Scope stats table ────────────────────────────────────────────────────
    {
        UI::Label hdr("Scope                      Avg(us)  Min(us)  Max(us)  Count");
        hdr.SetBounds({0, rowY, 700, 20});
        // [RENDER: hdr, colour = style.textHighlight]
        hdr.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 22;

        auto allStats = prof.AllScopeStats();
        // Sort by avg descending so hottest scopes appear first
        std::sort(allStats.begin(), allStats.end(),
                  [](const ScopeStats& a, const ScopeStats& b) {
                      return a.avgUs > b.avgUs;
                  });

        for (const ScopeStats& s : allStats) {
            std::ostringstream row;
            row << std::left  << std::setw(26) << s.name
                << std::right << std::setw(8)  << std::fixed << std::setprecision(1) << s.avgUs
                << std::setw(9) << s.minUs
                << std::setw(9) << s.maxUs
                << std::setw(7) << s.sampleCount;

            UI::Label scopeLbl(row.str());
            scopeLbl.SetBounds({0, rowY, 700, 18});
            // [RENDER: scopeLbl, colour = style.textPrimary]
            scopeLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
            rowY += 20;
        }
        rowY += 8;
    }

    // ── Last-frame sample list ───────────────────────────────────────────────
    {
        UI::Label hdr2("Last frame samples:");
        hdr2.SetBounds({0, rowY, 400, 20});
        // [RENDER: hdr2, colour = style.textSecondary]
        hdr2.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 22;

        auto samples = prof.LastFrameSamples();
        for (const ProfileSample& ps : samples) {
            std::string indent(static_cast<size_t>(ps.depth * 2), ' ');
            std::ostringstream row;
            row << std::left << std::setw(30) << (indent + ps.name)
                << std::fixed << std::setprecision(1) << ps.durationUs << " us";

            UI::Label sLbl(row.str());
            sLbl.SetBounds({0, rowY, 600, 18});
            // [RENDER: sLbl, colour = style.textPrimary]
            sLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
            rowY += 20;
        }
    }
}

} // namespace Tools
