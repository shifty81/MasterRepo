#include "Tools/GUI/panels/GPUProfilerPanel.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "Tools/Profiler/GPUProfiler/GPUProfiler.h"
#include "UI/Widgets/Widget.h"

namespace Tools {

// ── Impl ─────────────────────────────────────────────────────────────────────

struct GPUProfilerPanel::Impl {
    GPUProfiler* profiler{nullptr};
    static constexpr int kBarWidth = 20; // ASCII bar chart width
};

// ── Construction ──────────────────────────────────────────────────────────────

GPUProfilerPanel::GPUProfilerPanel() : m_impl(new Impl{}) {}
GPUProfilerPanel::~GPUProfilerPanel() { delete m_impl; }

void GPUProfilerPanel::OnAttach() {}
void GPUProfilerPanel::OnDetach() {}
void GPUProfilerPanel::OnUpdate(float /*dt*/) {}

void GPUProfilerPanel::SetProfiler(GPUProfiler* profiler) {
    m_impl->profiler = profiler;
}

// ── Draw ──────────────────────────────────────────────────────────────────────

void GPUProfilerPanel::OnDraw(const UI::UIStyle& style) {
    // [RENDER: panel background = style.panelBackground]

    // ── Title ────────────────────────────────────────────────────────────────
    UI::Label title("GPU Profiler");
    title.SetBounds({0, 0, 400, 24});
    // [RENDER: title label, colour = style.textHighlight, fontSizeLarge]
    title.OnEvent(UI::WidgetEvent::None, 0, 0);

    if (!m_impl->profiler) {
        UI::Label noData("No GPUProfiler attached.");
        noData.SetBounds({0, 30, 400, 20});
        // [RENDER: noData label, colour = style.textSecondary]
        noData.OnEvent(UI::WidgetEvent::None, 0, 30);
        return;
    }

    GPUProfiler& prof = *m_impl->profiler;
    const GPUFrame* last = prof.GetLastFrame();

    float rowY = 30.f;

    // ── Last-frame summary ───────────────────────────────────────────────────
    {
        std::ostringstream ss;
        ss << "Frame GPU time: ";
        if (last) ss << last->totalGPUTimeMs << " ms";
        else      ss << "N/A";

        UI::Label frameLbl(ss.str());
        frameLbl.SetBounds({0, rowY, 500, 20});
        // [RENDER: frameLbl, colour = style.textPrimary]
        frameLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 24;
    }

    // ── Peak / average ───────────────────────────────────────────────────────
    {
        double avg  = prof.AverageFrameTimeMs();
        double peak = prof.PeakFrameTimeMs();
        std::ostringstream ss;
        ss << "Avg: " << avg << " ms   Peak: " << peak << " ms";
        UI::Label statsLbl(ss.str());
        statsLbl.SetBounds({0, rowY, 500, 20});
        // [RENDER: statsLbl, colour = style.textSecondary]
        statsLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 28;
    }

    // ── Zone table ───────────────────────────────────────────────────────────
    if (last && !last->zones.empty()) {
        UI::Label hdr("Zone                       Time (ms)");
        hdr.SetBounds({0, rowY, 500, 20});
        // [RENDER: hdr, colour = style.textHighlight, bold]
        hdr.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 22;

        for (const GPUZone& z : last->zones) {
            std::string indent(static_cast<size_t>(z.depth * 2), ' ');
            std::ostringstream row;
            row.width(26);
            row << std::left << (indent + z.label);
            row << z.durationMs;

            UI::Label zoneLbl(row.str());
            zoneLbl.SetBounds({0, rowY, 500, 18});
            // [RENDER: zoneLbl, colour = style.textPrimary]
            zoneLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
            rowY += 20;
        }
        rowY += 8;
    }

    // ── ASCII bar chart — last N frame times ─────────────────────────────────
    {
        auto history = prof.GetFrameHistory(Impl::kBarWidth);
        if (!history.empty()) {
            UI::Label barTitle("Frame history:");
            barTitle.SetBounds({0, rowY, 300, 20});
            // [RENDER: barTitle, colour = style.textSecondary]
            barTitle.OnEvent(UI::WidgetEvent::None, 0, rowY);
            rowY += 22;

            double maxTime = 0.0;
            for (const auto* f : history) if (f) maxTime = std::max(maxTime, f->totalGPUTimeMs);
            if (maxTime <= 0.0) maxTime = 1.0;

            std::string bar;
            bar.reserve(history.size() * 2);
            for (const auto* f : history) {
                double t  = f ? f->totalGPUTimeMs : 0.0;
                int    h  = static_cast<int>((t / maxTime) * 8);
                // 8 block levels: ▁▂▃▄▅▆▇█
                static const char* kBlocks[] = {" ","▁","▂","▃","▄","▅","▆","▇","█"};
                bar += kBlocks[std::clamp(h, 0, 8)];
                bar += ' ';
            }
            UI::Label barLbl(bar);
            barLbl.SetBounds({0, rowY, 600, 20});
            // [RENDER: barLbl using monospace font, colour = style.accent]
            barLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
        }
    }
}

} // namespace Tools
