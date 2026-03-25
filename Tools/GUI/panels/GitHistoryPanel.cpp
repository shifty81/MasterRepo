#include "Tools/GUI/panels/GitHistoryPanel.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "Tools/GitPanel/GitHistory/GitHistory.h"
#include "UI/Widgets/Widget.h"

namespace Tools {

struct GitHistoryPanel::Impl {
    GitHistory*               history{nullptr};
    std::string               repoBuf;
    std::string               authorFilterBuf;
    std::string               keywordFilterBuf;
    std::vector<CommitRecord> displayCommits;
    bool                      loaded{false};
    int                       selectedCommitIdx{-1};
    std::string               diffText;
};

GitHistoryPanel::GitHistoryPanel() : m_impl(new Impl{}) {}
GitHistoryPanel::~GitHistoryPanel() { delete m_impl; }

void GitHistoryPanel::OnAttach()  {}
void GitHistoryPanel::OnDetach()  {}
void GitHistoryPanel::OnUpdate(float /*dt*/) {}

void GitHistoryPanel::SetHistory(GitHistory* history) {
    m_impl->history = history;
}

void GitHistoryPanel::OnDraw(const UI::UIStyle& style) {
    // [RENDER: panel background = style.panelBackground]

    UI::Label title("Git History");
    title.SetBounds({0, 0, 400, 24});
    // [RENDER: title, colour = style.textHighlight, fontSizeLarge]
    title.OnEvent(UI::WidgetEvent::None, 0, 0);

    float rowY = 30.f;

    // ── Repo path + Load ─────────────────────────────────────────────────────
    {
        UI::Label repoLbl("Repo:");
        repoLbl.SetBounds({0, rowY, 50, 20});
        // [RENDER: repoLbl, colour = style.textSecondary]
        repoLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);

        UI::TextInput repoInput;
        repoInput.SetValue(m_impl->repoBuf);
        repoInput.SetBounds({58, rowY, 340, 22});
        repoInput.SetOnTextChanged([this](const std::string& t) {
            m_impl->repoBuf = t;
        });
        // [RENDER: repoInput text box]
        repoInput.OnEvent(UI::WidgetEvent::None, 58, rowY);

        UI::Button loadBtn("Load");
        loadBtn.SetBounds({408, rowY, 70, 22});
        loadBtn.SetOnClick([this]() {
            if (!m_impl->history || m_impl->repoBuf.empty()) return;
            bool ok = m_impl->history->Load(m_impl->repoBuf, 200);
            if (ok) {
                m_impl->loaded         = true;
                m_impl->displayCommits = m_impl->history->AllCommits();
                m_impl->selectedCommitIdx = -1;
                m_impl->diffText.clear();
            }
        });
        // [RENDER: loadBtn, colour = style.accent]
        loadBtn.OnEvent(UI::WidgetEvent::None, 408, rowY);
        rowY += 32;
    }

    if (!m_impl->loaded) return;

    // ── Filter controls ──────────────────────────────────────────────────────
    {
        UI::Label aLbl("Author:");
        aLbl.SetBounds({0, rowY, 60, 20});
        // [RENDER: aLbl, colour = style.textSecondary]
        aLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);

        UI::TextInput aInput;
        aInput.SetValue(m_impl->authorFilterBuf);
        aInput.SetBounds({68, rowY, 160, 22});
        aInput.SetOnTextChanged([this](const std::string& t) {
            m_impl->authorFilterBuf = t;
        });
        // [RENDER: aInput text box]
        aInput.OnEvent(UI::WidgetEvent::None, 68, rowY);

        UI::Label kLbl("Keyword:");
        kLbl.SetBounds({238, rowY, 70, 20});
        // [RENDER: kLbl, colour = style.textSecondary]
        kLbl.OnEvent(UI::WidgetEvent::None, 238, rowY);

        UI::TextInput kInput;
        kInput.SetValue(m_impl->keywordFilterBuf);
        kInput.SetBounds({316, rowY, 160, 22});
        kInput.SetOnTextChanged([this](const std::string& t) {
            m_impl->keywordFilterBuf = t;
        });
        // [RENDER: kInput text box]
        kInput.OnEvent(UI::WidgetEvent::None, 316, rowY);

        UI::Button filterBtn("Filter");
        filterBtn.SetBounds({486, rowY, 70, 22});
        filterBtn.SetOnClick([this]() {
            if (!m_impl->history) return;
            HistoryFilter hf;
            hf.authorFilter  = m_impl->authorFilterBuf;
            hf.keywordFilter = m_impl->keywordFilterBuf;
            if (hf.authorFilter.empty() && hf.keywordFilter.empty())
                m_impl->displayCommits = m_impl->history->AllCommits();
            else
                m_impl->displayCommits = m_impl->history->Filter(hf);
            m_impl->selectedCommitIdx = -1;
            m_impl->diffText.clear();
        });
        // [RENDER: filterBtn, colour = style.buttonNormal]
        filterBtn.OnEvent(UI::WidgetEvent::None, 486, rowY);
        rowY += 32;
    }

    // ── Commit table ─────────────────────────────────────────────────────────
    {
        UI::Label hdr("SHA      Author               Date                 Subject");
        hdr.SetBounds({0, rowY, 750, 20});
        // [RENDER: hdr, colour = style.textHighlight]
        hdr.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 22;

        int maxRows = 15;
        for (int i = 0; i < std::min(maxRows, (int)m_impl->displayCommits.size()); ++i) {
            const CommitRecord& c = m_impl->displayCommits[i];

            // Format timestamp
            time_t ts = static_cast<time_t>(c.timestamp);
            std::tm* tm_info = std::gmtime(&ts);
            char dateBuf[20] = "unknown";
            if (tm_info) std::strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d %H:%M", tm_info);

            std::string subject = c.subject;
            if (subject.size() > 30) subject = subject.substr(0, 27) + "...";

            std::ostringstream row;
            row << std::left
                << std::setw(9)  << c.shortSha
                << std::setw(21) << c.author.substr(0, 20)
                << std::setw(21) << dateBuf
                << subject;

            UI::Label rowLbl(row.str());
            rowLbl.SetBounds({0, rowY, 750, 18});
            // [RENDER: rowLbl, highlight if i==selectedCommitIdx, colour = style.textPrimary]
            rowLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);

            // Simulate click: if mouse lands on this row, select it
            // In a real backend, the row widget would handle OnEvent(Clicked)
            // and set selectedCommitIdx = i; we wire a button for demonstration.
            UI::Button selBtn(">");
            selBtn.SetBounds({720, rowY, 20, 18});
            selBtn.SetOnClick([this, i]() {
                m_impl->selectedCommitIdx = i;
                if (m_impl->history) {
                    m_impl->diffText = m_impl->history->Diff(
                        m_impl->displayCommits[i].sha);
                }
            });
            // [RENDER: selBtn only visible on hover]
            selBtn.OnEvent(UI::WidgetEvent::None, 720, rowY);

            rowY += 20;
        }
        rowY += 8;
    }

    // ── Diff view ────────────────────────────────────────────────────────────
    if (!m_impl->diffText.empty()) {
        UI::Label diffHdr("Diff:");
        diffHdr.SetBounds({0, rowY, 100, 20});
        // [RENDER: diffHdr, colour = style.textHighlight]
        diffHdr.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 22;

        // Show first 20 lines of diff
        std::istringstream iss(m_impl->diffText);
        std::string line;
        int nLines = 0;
        while (std::getline(iss, line) && nLines < 20) {
            UI::Label dLbl(line);
            dLbl.SetBounds({0, rowY, 750, 16});
            // [RENDER: dLbl, colour = line starts '+' ? style.statusOk :
            //          line starts '-' ? style.statusError : style.textPrimary]
            dLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
            rowY += 17;
            ++nLines;
        }
    }
}

} // namespace Tools
