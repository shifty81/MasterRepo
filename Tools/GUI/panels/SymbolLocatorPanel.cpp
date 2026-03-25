#include "Tools/GUI/panels/SymbolLocatorPanel.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "Tools/Locator/SymbolLocator/SymbolLocator.h"
#include "UI/Widgets/Widget.h"

namespace Tools {

struct SymbolLocatorPanel::Impl {
    SymbolLocator*           locator{nullptr};
    std::string              queryBuf;
    std::vector<SymbolEntry> results;
    SymbolEntry              gotoResult;
    bool                     hasGoto{false};
    bool                     indexBuilt{false};
};

SymbolLocatorPanel::SymbolLocatorPanel() : m_impl(new Impl{}) {}
SymbolLocatorPanel::~SymbolLocatorPanel() { delete m_impl; }

void SymbolLocatorPanel::OnAttach()  {}
void SymbolLocatorPanel::OnDetach()  {}
void SymbolLocatorPanel::OnUpdate(float /*dt*/) {}

void SymbolLocatorPanel::SetLocator(SymbolLocator* locator) {
    m_impl->locator = locator;
}

void SymbolLocatorPanel::OnDraw(const UI::UIStyle& style) {
    // [RENDER: panel background = style.panelBackground]

    UI::Label title("Symbol Locator");
    title.SetBounds({0, 0, 400, 24});
    // [RENDER: title, colour = style.textHighlight, fontSizeLarge]
    title.OnEvent(UI::WidgetEvent::None, 0, 0);

    float rowY = 30.f;

    // ── Build Index button ───────────────────────────────────────────────────
    {
        UI::Button buildBtn("Build Index");
        buildBtn.SetBounds({0, rowY, 120, 22});
        buildBtn.SetOnClick([this]() {
            if (!m_impl->locator) return;
            m_impl->locator->BuildIndex();
            m_impl->indexBuilt = true;
        });
        // [RENDER: buildBtn, colour = style.accent]
        buildBtn.OnEvent(UI::WidgetEvent::None, 0, rowY);

        if (m_impl->indexBuilt && m_impl->locator) {
            auto stats = m_impl->locator->Stats();
            std::ostringstream ss;
            ss << "  " << stats.symbolCount << " symbols in "
               << stats.filesIndexed << " files";
            UI::Label statsLbl(ss.str());
            statsLbl.SetBounds({130, rowY, 400, 22});
            // [RENDER: statsLbl, colour = style.textSecondary]
            statsLbl.OnEvent(UI::WidgetEvent::None, 130, rowY);
        }
        rowY += 32;
    }

    // ── Search input ─────────────────────────────────────────────────────────
    {
        UI::Label qLbl("Search:");
        qLbl.SetBounds({0, rowY, 70, 20});
        // [RENDER: qLbl, colour = style.textSecondary]
        qLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);

        UI::TextInput qInput;
        qInput.SetValue(m_impl->queryBuf);
        qInput.SetBounds({80, rowY, 300, 22});
        qInput.SetOnTextChanged([this](const std::string& t) {
            m_impl->queryBuf = t;
        });
        // [RENDER: qInput text box]
        qInput.OnEvent(UI::WidgetEvent::None, 80, rowY);

        UI::Button searchBtn("Search");
        searchBtn.SetBounds({390, rowY, 80, 22});
        searchBtn.SetOnClick([this]() {
            if (!m_impl->locator || m_impl->queryBuf.empty()) return;
            m_impl->results  = m_impl->locator->Search(m_impl->queryBuf, 50);
            m_impl->gotoResult = m_impl->locator->GoToDefinition(m_impl->queryBuf);
            m_impl->hasGoto  = !m_impl->gotoResult.name.empty();
        });
        // [RENDER: searchBtn, colour = style.accent]
        searchBtn.OnEvent(UI::WidgetEvent::None, 390, rowY);
        rowY += 32;
    }

    // ── GoToDefinition result ────────────────────────────────────────────────
    if (m_impl->hasGoto) {
        const SymbolEntry& g = m_impl->gotoResult;
        std::ostringstream ss;
        ss << "GoTo: " << g.name << " [" << SymbolKindName(g.kind) << "] "
           << g.filePath << ":" << g.line;
        UI::Label gLbl(ss.str());
        gLbl.SetBounds({0, rowY, 700, 20});
        // [RENDER: gLbl, colour = style.accentHover]
        gLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 28;
    }

    // ── Results list ─────────────────────────────────────────────────────────
    if (!m_impl->results.empty()) {
        UI::Label hdr("Name                       Kind        File                  Line");
        hdr.SetBounds({0, rowY, 700, 20});
        // [RENDER: hdr, colour = style.textHighlight]
        hdr.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 22;

        for (const SymbolEntry& e : m_impl->results) {
            // Truncate file path for display
            std::string file = e.filePath;
            if (file.size() > 22) file = "..." + file.substr(file.size() - 19);

            std::ostringstream row;
            row << std::left  << std::setw(26) << e.name
                << std::setw(12) << SymbolKindName(e.kind)
                << std::setw(22) << file
                << e.line;

            UI::Label rLbl(row.str());
            rLbl.SetBounds({0, rowY, 700, 18});
            // [RENDER: rLbl, colour = style.textPrimary]
            rLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
            rowY += 20;
        }
    }
}

} // namespace Tools
