#include "Tools/GUI/panels/MemoryProfilerPanel.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "Tools/MemoryProfiler/MemoryProfiler.h"
#include "UI/Widgets/Widget.h"

namespace Tools {

struct MemoryProfilerPanel::Impl {
    MemoryProfiler*               profiler{nullptr};
    std::vector<MemorySnapshot>   snapshots;
    std::vector<std::string>      snapshotLabels;
    int                           diffIdxA{0};
    int                           diffIdxB{1};
    std::string                   snapshotLabelBuf{"snapshot"};
};

MemoryProfilerPanel::MemoryProfilerPanel() : m_impl(new Impl{}) {}
MemoryProfilerPanel::~MemoryProfilerPanel() { delete m_impl; }

void MemoryProfilerPanel::OnAttach()  {}
void MemoryProfilerPanel::OnDetach()  {}
void MemoryProfilerPanel::OnUpdate(float /*dt*/) {}

void MemoryProfilerPanel::SetProfiler(MemoryProfiler* profiler) {
    m_impl->profiler = profiler;
}

void MemoryProfilerPanel::OnDraw(const UI::UIStyle& style) {
    // [RENDER: panel background = style.panelBackground]

    UI::Label title("Memory Profiler");
    title.SetBounds({0, 0, 400, 24});
    // [RENDER: title, colour = style.textHighlight, fontSizeLarge]
    title.OnEvent(UI::WidgetEvent::None, 0, 0);

    if (!m_impl->profiler) {
        UI::Label noData("No MemoryProfiler attached.");
        noData.SetBounds({0, 30, 400, 20});
        noData.OnEvent(UI::WidgetEvent::None, 0, 30);
        return;
    }

    MemoryProfiler& prof = *m_impl->profiler;
    float rowY = 30.f;

    // ── Total live bytes ─────────────────────────────────────────────────────
    {
        std::ostringstream ss;
        ss << "Total live: " << (prof.LiveBytes() / 1024) << " KB";
        UI::Label lbl(ss.str());
        lbl.SetBounds({0, rowY, 400, 20});
        // [RENDER: lbl, colour = style.statusOk]
        lbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 26;
    }

    // ── Per-tag table ────────────────────────────────────────────────────────
    {
        UI::Label hdr("Tag                        Live(KB)  Peak(KB)  Count");
        hdr.SetBounds({0, rowY, 600, 20});
        // [RENDER: hdr, colour = style.textHighlight]
        hdr.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 22;

        auto tags = prof.AllTagStats();
        std::sort(tags.begin(), tags.end(),
                  [](const TagStats& a, const TagStats& b) {
                      return a.liveBytes > b.liveBytes;
                  });

        for (const TagStats& ts : tags) {
            std::ostringstream row;
            row << std::left  << std::setw(26) << ts.tag
                << std::right << std::setw(9)  << (ts.liveBytes / 1024)
                << std::setw(10) << (ts.peakBytes / 1024)
                << std::setw(7)  << ts.allocationCount;

            UI::Label lbl(row.str());
            lbl.SetBounds({0, rowY, 600, 18});
            // [RENDER: lbl, colour = style.textPrimary]
            lbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
            rowY += 20;
        }
        rowY += 8;
    }

    // ── Snapshot controls ────────────────────────────────────────────────────
    {
        UI::TextInput labelInput;
        labelInput.SetValue(m_impl->snapshotLabelBuf);
        labelInput.SetBounds({0, rowY, 200, 22});
        labelInput.SetOnTextChanged([this](const std::string& t) {
            m_impl->snapshotLabelBuf = t;
        });
        // [RENDER: labelInput text box]
        labelInput.OnEvent(UI::WidgetEvent::None, 0, rowY);

        UI::Button snapBtn("Take Snapshot");
        snapBtn.SetBounds({210, rowY, 130, 22});
        snapBtn.SetOnClick([this]() {
            if (!m_impl->profiler) return;
            auto snap = m_impl->profiler->Snapshot(m_impl->snapshotLabelBuf);
            m_impl->snapshots.push_back(snap);
            m_impl->snapshotLabels.push_back(
                snap.label.empty() ? std::string("snap") + std::to_string(m_impl->snapshots.size())
                                   : snap.label);
        });
        // [RENDER: snapBtn, colour = style.accent]
        snapBtn.OnEvent(UI::WidgetEvent::None, 210, rowY);
        rowY += 30;
    }

    // ── Snapshot list ────────────────────────────────────────────────────────
    if (!m_impl->snapshots.empty()) {
        UI::Label snapHdr("Snapshots:");
        snapHdr.SetBounds({0, rowY, 300, 20});
        // [RENDER: snapHdr, colour = style.textSecondary]
        snapHdr.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 22;

        for (int i = 0; i < static_cast<int>(m_impl->snapshotLabels.size()); ++i) {
            std::string label = "[" + std::to_string(i) + "] " + m_impl->snapshotLabels[i];
            UI::Label sLbl(label);
            sLbl.SetBounds({0, rowY, 400, 18});
            // [RENDER: sLbl, colour = (i==diffIdxA||i==diffIdxB) ? style.accentHover : style.textPrimary]
            sLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
            rowY += 20;
        }
        rowY += 6;

        // Diff two snapshots
        if (m_impl->snapshots.size() >= 2) {
            UI::Label diffHdr("Diff (A vs B):");
            diffHdr.SetBounds({0, rowY, 300, 20});
            // [RENDER: diffHdr, colour = style.textHighlight]
            diffHdr.OnEvent(UI::WidgetEvent::None, 0, rowY);
            rowY += 22;

            int a = std::clamp(m_impl->diffIdxA, 0, (int)m_impl->snapshots.size() - 1);
            int b = std::clamp(m_impl->diffIdxB, 0, (int)m_impl->snapshots.size() - 1);
            if (a != b) {
                SnapshotDiff diff = DiffSnapshots(m_impl->snapshots[a], m_impl->snapshots[b]);

                for (const auto& [tag, delta] : diff.deltaByTag) {
                    std::ostringstream row;
                    row << std::left  << std::setw(26) << tag
                        << std::right << std::showpos << (static_cast<int64_t>(delta) / 1024)
                        << " KB";
                    UI::Label dLbl(row.str());
                    dLbl.SetBounds({0, rowY, 500, 18});
                    // [RENDER: dLbl, colour = delta>0 ? style.statusError : style.statusOk]
                    dLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
                    rowY += 20;
                }
            }
        }
    }
}

} // namespace Tools
