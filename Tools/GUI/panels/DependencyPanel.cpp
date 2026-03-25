#include "Tools/GUI/panels/DependencyPanel.h"

#include <sstream>
#include <string>

#include "Tools/DependencyAnalyzer/DependencyAnalyzer.h"
#include "UI/Widgets/Widget.h"

namespace Tools {

struct DependencyPanel::Impl {
    DependencyAnalyzer* analyzer{nullptr};
    std::string         searchRootBuf;
    DependencyReport    lastReport;
    bool                hasReport{false};
    bool                analysing{false};
};

DependencyPanel::DependencyPanel() : m_impl(new Impl{}) {}
DependencyPanel::~DependencyPanel() { delete m_impl; }

void DependencyPanel::OnAttach()  {}
void DependencyPanel::OnDetach()  {}
void DependencyPanel::OnUpdate(float /*dt*/) {}

void DependencyPanel::SetAnalyzer(DependencyAnalyzer* analyzer) {
    m_impl->analyzer = analyzer;
}

void DependencyPanel::OnDraw(const UI::UIStyle& style) {
    // [RENDER: panel background = style.panelBackground]

    UI::Label title("Dependency Analyzer");
    title.SetBounds({0, 0, 400, 24});
    // [RENDER: title, colour = style.textHighlight, fontSizeLarge]
    title.OnEvent(UI::WidgetEvent::None, 0, 0);

    float rowY = 30.f;

    // ── Search root input ────────────────────────────────────────────────────
    {
        UI::Label rootLbl("Search root:");
        rootLbl.SetBounds({0, rowY, 100, 20});
        // [RENDER: rootLbl, colour = style.textSecondary]
        rootLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);

        UI::TextInput rootInput;
        rootInput.SetValue(m_impl->searchRootBuf);
        rootInput.SetBounds({110, rowY, 340, 22});
        rootInput.SetOnTextChanged([this](const std::string& t) {
            m_impl->searchRootBuf = t;
        });
        // [RENDER: rootInput text box]
        rootInput.OnEvent(UI::WidgetEvent::None, 110, rowY);

        UI::Button analyseBtn("Analyse");
        analyseBtn.SetBounds({460, rowY, 90, 22});
        analyseBtn.SetOnClick([this]() {
            if (!m_impl->analyzer || m_impl->searchRootBuf.empty()) return;
            m_impl->analyzer->AddSearchRoot(m_impl->searchRootBuf);
            m_impl->lastReport = m_impl->analyzer->Analyse();
            m_impl->hasReport  = true;
        });
        // [RENDER: analyseBtn, colour = style.accent]
        analyseBtn.OnEvent(UI::WidgetEvent::None, 460, rowY);
        rowY += 32;
    }

    if (!m_impl->hasReport) return;

    const DependencyReport& rep = m_impl->lastReport;

    // ── Summary ──────────────────────────────────────────────────────────────
    {
        std::ostringstream ss;
        ss << "Files: " << rep.FileCount()
           << "   Cycles: " << rep.cycles.size()
           << (rep.HasCycles() ? "  [!]" : "  [OK]");
        UI::Label sumLbl(ss.str());
        sumLbl.SetBounds({0, rowY, 500, 20});
        // [RENDER: sumLbl, colour = rep.HasCycles() ? style.statusWarning : style.statusOk]
        sumLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 28;
    }

    // ── Top fanout ───────────────────────────────────────────────────────────
    {
        UI::Label hdr("Top Fanout (most includes):");
        hdr.SetBounds({0, rowY, 400, 20});
        // [RENDER: hdr, colour = style.textHighlight]
        hdr.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 22;

        std::string fanoutText = rep.TopFanout(8);
        // Split by newline and display each line
        std::istringstream iss(fanoutText);
        std::string line;
        while (std::getline(iss, line)) {
            UI::Label lineLbl(line);
            lineLbl.SetBounds({0, rowY, 600, 18});
            // [RENDER: lineLbl, colour = style.textPrimary]
            lineLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
            rowY += 20;
        }
        rowY += 8;
    }

    // ── Top fanin ────────────────────────────────────────────────────────────
    {
        UI::Label hdr("Top Fanin (most included by):");
        hdr.SetBounds({0, rowY, 400, 20});
        // [RENDER: hdr, colour = style.textHighlight]
        hdr.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 22;

        std::string faninText = rep.TopFanin(8);
        std::istringstream iss(faninText);
        std::string line;
        while (std::getline(iss, line)) {
            UI::Label lineLbl(line);
            lineLbl.SetBounds({0, rowY, 600, 18});
            // [RENDER: lineLbl, colour = style.textPrimary]
            lineLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
            rowY += 20;
        }
        rowY += 8;
    }

    // ── Cycle list ───────────────────────────────────────────────────────────
    if (!rep.cycles.empty()) {
        UI::Label cycleHdr("Include cycles:");
        cycleHdr.SetBounds({0, rowY, 400, 20});
        // [RENDER: cycleHdr, colour = style.statusError]
        cycleHdr.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 22;

        for (const auto& cycle : rep.cycles) {
            std::ostringstream ss;
            for (size_t i = 0; i < cycle.chain.size(); ++i) {
                if (i) ss << " -> ";
                // Extract just the filename for readability
                size_t slash = cycle.chain[i].rfind('/');
                if (slash == std::string::npos) slash = cycle.chain[i].rfind('\\');
                ss << (slash != std::string::npos ? cycle.chain[i].substr(slash + 1)
                                                  : cycle.chain[i]);
            }
            UI::Label cLbl(ss.str());
            cLbl.SetBounds({0, rowY, 700, 18});
            // [RENDER: cLbl, colour = style.statusWarning]
            cLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
            rowY += 20;
        }
    }
}

} // namespace Tools
