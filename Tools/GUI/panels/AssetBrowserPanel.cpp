#include "Tools/GUI/panels/AssetBrowserPanel.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "Tools/AssetTools/AssetBrowser/AssetBrowser.h"
#include "UI/Widgets/Widget.h"

namespace Tools {

struct AssetBrowserPanel::Impl {
    AssetBrowser* browser{nullptr};
    std::string   rootPathBuf;
    std::string   filterBuf;
    std::string   renameBuf;
    std::string   importDestBuf;
    bool          showRenameInput{false};

    static std::string TypeIcon(const AssetEntry& e) {
        if (e.isDirectory)            return "[DIR]";
        if (e.extension == ".png" ||
            e.extension == ".jpg" ||
            e.extension == ".tga")    return "[IMG]";
        if (e.extension == ".wav" ||
            e.extension == ".ogg" ||
            e.extension == ".mp3")    return "[SND]";
        if (e.extension == ".cpp" ||
            e.extension == ".h"   ||
            e.extension == ".lua")    return "[SRC]";
        return "[FILE]";
    }

    static std::string FormatSize(uint64_t bytes) {
        if (bytes >= 1024 * 1024)
            return std::to_string(bytes / (1024 * 1024)) + " MB";
        if (bytes >= 1024)
            return std::to_string(bytes / 1024) + " KB";
        return std::to_string(bytes) + " B";
    }
};

AssetBrowserPanel::AssetBrowserPanel() : m_impl(new Impl{}) {}
AssetBrowserPanel::~AssetBrowserPanel() { delete m_impl; }

void AssetBrowserPanel::OnAttach()  {}
void AssetBrowserPanel::OnDetach()  {}
void AssetBrowserPanel::OnUpdate(float /*dt*/) {}

void AssetBrowserPanel::SetBrowser(AssetBrowser* browser) {
    m_impl->browser = browser;
}

void AssetBrowserPanel::OnDraw(const UI::UIStyle& style) {
    // [RENDER: panel background = style.panelBackground]

    UI::Label title("Asset Browser");
    title.SetBounds({0, 0, 400, 24});
    // [RENDER: title, colour = style.textHighlight, fontSizeLarge]
    title.OnEvent(UI::WidgetEvent::None, 0, 0);

    float rowY = 30.f;

    // ── Root path + Navigate bar ──────────────────────────────────────────────
    {
        UI::Label rootLbl("Root:");
        rootLbl.SetBounds({0, rowY, 45, 20});
        // [RENDER: rootLbl, colour = style.textSecondary]
        rootLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);

        UI::TextInput rootInput;
        rootInput.SetValue(m_impl->rootPathBuf);
        rootInput.SetBounds({48, rowY, 300, 22});
        rootInput.SetOnTextChanged([this](const std::string& t) {
            m_impl->rootPathBuf = t;
        });
        // [RENDER: rootInput]
        rootInput.OnEvent(UI::WidgetEvent::None, 48, rowY);

        UI::Button goBtn("Go");
        goBtn.SetBounds({356, rowY, 40, 22});
        goBtn.SetOnClick([this]() {
            if (!m_impl->browser || m_impl->rootPathBuf.empty()) return;
            m_impl->browser->SetRootPath(m_impl->rootPathBuf);
        });
        // [RENDER: goBtn, colour = style.accent]
        goBtn.OnEvent(UI::WidgetEvent::None, 356, rowY);

        UI::Button upBtn("Up");
        upBtn.SetBounds({402, rowY, 40, 22});
        upBtn.SetOnClick([this]() {
            if (m_impl->browser) m_impl->browser->NavigateUp();
        });
        // [RENDER: upBtn, colour = style.buttonNormal]
        upBtn.OnEvent(UI::WidgetEvent::None, 402, rowY);

        UI::Button backBtn("<");
        backBtn.SetBounds({448, rowY, 28, 22});
        backBtn.SetOnClick([this]() {
            if (m_impl->browser) m_impl->browser->NavigateBack();
        });
        backBtn.OnEvent(UI::WidgetEvent::None, 448, rowY);

        UI::Button fwdBtn(">");
        fwdBtn.SetBounds({480, rowY, 28, 22});
        fwdBtn.SetOnClick([this]() {
            if (m_impl->browser) m_impl->browser->NavigateForward();
        });
        fwdBtn.OnEvent(UI::WidgetEvent::None, 480, rowY);
        rowY += 32;
    }

    // ── Current path display ──────────────────────────────────────────────────
    if (m_impl->browser) {
        UI::Label curLbl("Path: " + m_impl->browser->CurrentPath());
        curLbl.SetBounds({0, rowY, 700, 20});
        // [RENDER: curLbl, colour = style.textSecondary]
        curLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 26;
    }

    // ── Filter input ─────────────────────────────────────────────────────────
    {
        UI::Label fLbl("Filter:");
        fLbl.SetBounds({0, rowY, 50, 20});
        // [RENDER: fLbl, colour = style.textSecondary]
        fLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);

        UI::TextInput fInput;
        fInput.SetValue(m_impl->filterBuf);
        fInput.SetBounds({56, rowY, 200, 22});
        fInput.SetOnTextChanged([this](const std::string& t) {
            m_impl->filterBuf = t;
            if (!m_impl->browser) return;
            if (t.empty()) {
                m_impl->browser->ClearFilter();
            } else {
                AssetFilter af;
                af.nameSubstring = t;
                m_impl->browser->SetFilter(af);
            }
        });
        // [RENDER: fInput]
        fInput.OnEvent(UI::WidgetEvent::None, 56, rowY);

        UI::Button refreshBtn("Refresh");
        refreshBtn.SetBounds({264, rowY, 70, 22});
        refreshBtn.SetOnClick([this]() {
            if (m_impl->browser) m_impl->browser->Refresh();
        });
        // [RENDER: refreshBtn, colour = style.buttonNormal]
        refreshBtn.OnEvent(UI::WidgetEvent::None, 264, rowY);
        rowY += 32;
    }

    if (!m_impl->browser) return;

    // ── Entry list ───────────────────────────────────────────────────────────
    {
        UI::Label hdr("Type   Name                          Size");
        hdr.SetBounds({0, rowY, 600, 20});
        // [RENDER: hdr, colour = style.textHighlight]
        hdr.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 22;

        const auto& entries = m_impl->browser->Entries();
        int maxRows = 18;
        for (int i = 0; i < std::min(maxRows, (int)entries.size()); ++i) {
            const AssetEntry& e = entries[i];
            bool selected = (e.fullPath == m_impl->browser->SelectedPath());

            std::ostringstream row;
            row << std::left
                << std::setw(7)  << Impl::TypeIcon(e)
                << std::setw(32) << e.name.substr(0, 31)
                << (e.isDirectory ? "—" : Impl::FormatSize(e.sizeBytes));

            UI::Label eLbl(row.str());
            eLbl.SetBounds({0, rowY, 600, 18});
            // [RENDER: eLbl, bg = selected ? style.selectionBg : transparent,
            //          colour = style.textPrimary]
            eLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);

            // Select / navigate button
            UI::Button selBtn(e.isDirectory ? "Open" : "Sel");
            selBtn.SetBounds({610, rowY, 44, 18});
            selBtn.SetOnClick([this, i, &entries]() {
                const AssetEntry& entry = entries[i];
                if (entry.isDirectory) {
                    m_impl->browser->NavigateTo(entry.fullPath);
                } else {
                    m_impl->browser->Select(entry.fullPath);
                }
            });
            // [RENDER: selBtn, colour = style.accent]
            selBtn.OnEvent(UI::WidgetEvent::None, 610, rowY);
            rowY += 20;
        }
        if (entries.size() > static_cast<size_t>(maxRows)) {
            UI::Label moreLbl("... +" + std::to_string(entries.size() - maxRows) + " more");
            moreLbl.SetBounds({0, rowY, 300, 18});
            // [RENDER: moreLbl, colour = style.textDisabled]
            moreLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
            rowY += 20;
        }
        rowY += 6;
    }

    // ── Action buttons (for selected entry) ──────────────────────────────────
    {
        const std::string& sel = m_impl->browser->SelectedPath();

        UI::Button openBtn("Open");
        openBtn.SetBounds({0, rowY, 60, 22});
        openBtn.SetEnabled(!sel.empty());
        openBtn.SetOnClick([this]() {
            if (m_impl->browser) m_impl->browser->OpenAsset(m_impl->browser->SelectedPath());
        });
        // [RENDER: openBtn, colour = sel.empty() ? style.buttonDisabled : style.accent]
        openBtn.OnEvent(UI::WidgetEvent::None, 0, rowY);

        UI::Button delBtn("Delete");
        delBtn.SetBounds({68, rowY, 70, 22});
        delBtn.SetEnabled(!sel.empty());
        delBtn.SetOnClick([this]() {
            if (!m_impl->browser) return;
            const std::string& path = m_impl->browser->SelectedPath();
            if (!path.empty()) {
                m_impl->browser->DeleteAsset(path);
                m_impl->browser->ClearSelection();
            }
        });
        // [RENDER: delBtn, colour = sel.empty() ? style.buttonDisabled : style.statusError]
        delBtn.OnEvent(UI::WidgetEvent::None, 68, rowY);

        UI::Button renBtn("Rename");
        renBtn.SetBounds({146, rowY, 72, 22});
        renBtn.SetEnabled(!sel.empty());
        renBtn.SetOnClick([this]() {
            m_impl->showRenameInput = !m_impl->showRenameInput;
            if (m_impl->showRenameInput)
                m_impl->renameBuf = m_impl->browser ? m_impl->browser->SelectedPath() : "";
        });
        // [RENDER: renBtn, colour = sel.empty() ? style.buttonDisabled : style.buttonNormal]
        renBtn.OnEvent(UI::WidgetEvent::None, 146, rowY);

        UI::Button newFolderBtn("New Folder");
        newFolderBtn.SetBounds({226, rowY, 90, 22});
        newFolderBtn.SetOnClick([this]() {
            if (!m_impl->browser || m_impl->browser->CurrentPath().empty()) return;
            std::string newPath = m_impl->browser->CurrentPath() + "/NewFolder";
            m_impl->browser->CreateFolder(newPath);
        });
        // [RENDER: newFolderBtn, colour = style.buttonNormal]
        newFolderBtn.OnEvent(UI::WidgetEvent::None, 226, rowY);
        rowY += 32;
    }

    // ── Rename input ─────────────────────────────────────────────────────────
    if (m_impl->showRenameInput) {
        UI::TextInput renInput;
        renInput.SetValue(m_impl->renameBuf);
        renInput.SetBounds({0, rowY, 300, 22});
        renInput.SetOnTextChanged([this](const std::string& t) { m_impl->renameBuf = t; });
        // [RENDER: renInput]
        renInput.OnEvent(UI::WidgetEvent::None, 0, rowY);

        UI::Button applyBtn("Apply");
        applyBtn.SetBounds({308, rowY, 60, 22});
        applyBtn.SetOnClick([this]() {
            if (!m_impl->browser) return;
            const std::string& old = m_impl->browser->SelectedPath();
            if (!old.empty() && !m_impl->renameBuf.empty()) {
                m_impl->browser->RenameAsset(old, m_impl->renameBuf);
                m_impl->browser->ClearSelection();
            }
            m_impl->showRenameInput = false;
        });
        // [RENDER: applyBtn, colour = style.accent]
        applyBtn.OnEvent(UI::WidgetEvent::None, 308, rowY);
        rowY += 32;
    }

    // ── Stats bar ─────────────────────────────────────────────────────────────
    {
        AssetBrowserStats st = m_impl->browser->Stats();
        std::ostringstream ss;
        ss << "Files: " << st.totalFiles
           << "  Dirs: "  << st.totalDirs
           << "  Total: " << Impl::FormatSize(st.totalSizeBytes);

        UI::Label statsLbl(ss.str());
        statsLbl.SetBounds({0, rowY, 600, 18});
        // [RENDER: statsLbl, colour = style.textSecondary]
        statsLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
    }
}

} // namespace Tools
