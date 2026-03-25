#include "Tools/GUI/panels/AtlasPackerPanel.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "Tools/Packing/TextureAtlasPacker/TextureAtlasPacker.h"
#include "UI/Widgets/Widget.h"

namespace Tools {

struct AtlasPackerPanel::Impl {
    TextureAtlasPacker* packer{nullptr};

    AtlasConfig              config;
    std::vector<TextureEntry> pendingTextures;

    // Text buffer for adding a new texture
    std::string addNameBuf;
    std::string addWidthBuf{"64"};
    std::string addHeightBuf{"64"};

    PackResult   lastResult;
    bool         hasPacked{false};

    // Algo display names
    static const char* AlgoName(PackAlgo a) {
        return a == PackAlgo::Shelf ? "Shelf" : "MaxRects";
    }
};

AtlasPackerPanel::AtlasPackerPanel() : m_impl(new Impl{}) {}
AtlasPackerPanel::~AtlasPackerPanel() { delete m_impl; }

void AtlasPackerPanel::OnAttach()  {}
void AtlasPackerPanel::OnDetach()  {}
void AtlasPackerPanel::OnUpdate(float /*dt*/) {}

void AtlasPackerPanel::SetPacker(TextureAtlasPacker* packer) {
    m_impl->packer = packer;
}

void AtlasPackerPanel::OnDraw(const UI::UIStyle& style) {
    // [RENDER: panel background = style.panelBackground]

    UI::Label title("Texture Atlas Packer");
    title.SetBounds({0, 0, 400, 24});
    // [RENDER: title, colour = style.textHighlight, fontSizeLarge]
    title.OnEvent(UI::WidgetEvent::None, 0, 0);

    float rowY = 30.f;

    // ── Atlas config ─────────────────────────────────────────────────────────
    {
        // Page size display
        std::ostringstream ss;
        ss << "Page: " << m_impl->config.pageWidth << "x" << m_impl->config.pageHeight
           << "  Padding: " << m_impl->config.padding
           << "  Algo: " << Impl::AlgoName(m_impl->config.algo);
        UI::Label cfgLbl(ss.str());
        cfgLbl.SetBounds({0, rowY, 600, 20});
        // [RENDER: cfgLbl, colour = style.textSecondary]
        cfgLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 4;

        // Page size slider (powers of two: 256..4096 — simplified as float range)
        UI::Slider pageSizeSlider(256.f, 4096.f,
                                  static_cast<float>(m_impl->config.pageWidth));
        pageSizeSlider.SetBounds({0, rowY + 22, 200, 16});
        pageSizeSlider.SetOnChanged([this](float v) {
            uint32_t snapped = 256;
            while (snapped * 2 <= static_cast<uint32_t>(v)) snapped *= 2;
            m_impl->config.pageWidth = m_impl->config.pageHeight = snapped;
        });
        // [RENDER: pageSizeSlider labelled "Page size"]
        pageSizeSlider.OnEvent(UI::WidgetEvent::None, 0, rowY + 22);

        // Padding slider 0..16
        UI::Slider padSlider(0.f, 16.f, static_cast<float>(m_impl->config.padding));
        padSlider.SetBounds({220, rowY + 22, 120, 16});
        padSlider.SetOnChanged([this](float v) {
            m_impl->config.padding = static_cast<uint32_t>(v);
        });
        // [RENDER: padSlider labelled "Padding"]
        padSlider.OnEvent(UI::WidgetEvent::None, 220, rowY + 22);

        // Toggle algo
        UI::Button algoBtn(Impl::AlgoName(m_impl->config.algo));
        algoBtn.SetBounds({360, rowY + 20, 100, 22});
        algoBtn.SetOnClick([this]() {
            m_impl->config.algo = (m_impl->config.algo == PackAlgo::Shelf)
                                  ? PackAlgo::MaxRects : PackAlgo::Shelf;
        });
        // [RENDER: algoBtn, colour = style.buttonNormal]
        algoBtn.OnEvent(UI::WidgetEvent::None, 360, rowY + 20);

        // Allow rotation checkbox
        UI::Checkbox rotCb;
        rotCb.SetChecked(m_impl->config.allowRotation);
        rotCb.SetBounds({470, rowY + 22, 20, 20});
        rotCb.SetOnChanged([this](bool v) { m_impl->config.allowRotation = v; });
        // [RENDER: rotCb labelled "Allow rotation"]
        rotCb.OnEvent(UI::WidgetEvent::None, 470, rowY + 22);

        rowY += 52;
    }

    // ── Add texture row ───────────────────────────────────────────────────────
    {
        UI::Label nLbl("Name:");
        nLbl.SetBounds({0, rowY, 45, 20});
        nLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);

        UI::TextInput nInput;
        nInput.SetValue(m_impl->addNameBuf);
        nInput.SetBounds({48, rowY, 140, 22});
        nInput.SetOnTextChanged([this](const std::string& t) { m_impl->addNameBuf = t; });
        // [RENDER: nInput]
        nInput.OnEvent(UI::WidgetEvent::None, 48, rowY);

        UI::Label wLbl("W:");
        wLbl.SetBounds({196, rowY, 20, 20});
        wLbl.OnEvent(UI::WidgetEvent::None, 196, rowY);

        UI::TextInput wInput;
        wInput.SetValue(m_impl->addWidthBuf);
        wInput.SetBounds({218, rowY, 60, 22});
        wInput.SetOnTextChanged([this](const std::string& t) { m_impl->addWidthBuf = t; });
        // [RENDER: wInput]
        wInput.OnEvent(UI::WidgetEvent::None, 218, rowY);

        UI::Label hLbl("H:");
        hLbl.SetBounds({284, rowY, 20, 20});
        hLbl.OnEvent(UI::WidgetEvent::None, 284, rowY);

        UI::TextInput hInput;
        hInput.SetValue(m_impl->addHeightBuf);
        hInput.SetBounds({306, rowY, 60, 22});
        hInput.SetOnTextChanged([this](const std::string& t) { m_impl->addHeightBuf = t; });
        // [RENDER: hInput]
        hInput.OnEvent(UI::WidgetEvent::None, 306, rowY);

        UI::Button addBtn("Add");
        addBtn.SetBounds({374, rowY, 50, 22});
        addBtn.SetOnClick([this]() {
            if (m_impl->addNameBuf.empty()) return;
            TextureEntry e;
            e.name   = m_impl->addNameBuf;
            e.width  = static_cast<uint32_t>(std::stoul(m_impl->addWidthBuf.empty()  ? "0" : m_impl->addWidthBuf));
            e.height = static_cast<uint32_t>(std::stoul(m_impl->addHeightBuf.empty() ? "0" : m_impl->addHeightBuf));
            m_impl->pendingTextures.push_back(e);
            if (m_impl->packer) m_impl->packer->AddTexture(e);
            m_impl->addNameBuf.clear();
        });
        // [RENDER: addBtn, colour = style.accent]
        addBtn.OnEvent(UI::WidgetEvent::None, 374, rowY);

        UI::Button clearBtn("Clear All");
        clearBtn.SetBounds({432, rowY, 80, 22});
        clearBtn.SetOnClick([this]() {
            m_impl->pendingTextures.clear();
            m_impl->hasPacked = false;
            if (m_impl->packer) m_impl->packer->ClearTextures();
        });
        // [RENDER: clearBtn, colour = style.buttonNormal]
        clearBtn.OnEvent(UI::WidgetEvent::None, 432, rowY);
        rowY += 32;
    }

    // ── Texture list ─────────────────────────────────────────────────────────
    if (!m_impl->pendingTextures.empty()) {
        UI::Label hdr("Textures:");
        hdr.SetBounds({0, rowY, 300, 20});
        // [RENDER: hdr, colour = style.textSecondary]
        hdr.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 22;

        for (int i = 0; i < static_cast<int>(m_impl->pendingTextures.size()); ++i) {
            const TextureEntry& te = m_impl->pendingTextures[i];
            std::ostringstream ss;
            ss << std::left << std::setw(24) << te.name
               << te.width << " x " << te.height;
            UI::Label tLbl(ss.str());
            tLbl.SetBounds({0, rowY, 400, 18});
            // [RENDER: tLbl, colour = style.textPrimary]
            tLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);

            // Remove button per row
            UI::Button remBtn("X");
            remBtn.SetBounds({420, rowY, 24, 18});
            remBtn.SetOnClick([this, i]() {
                if (i < static_cast<int>(m_impl->pendingTextures.size())) {
                    if (m_impl->packer)
                        m_impl->packer->RemoveTexture(m_impl->pendingTextures[i].name);
                    m_impl->pendingTextures.erase(m_impl->pendingTextures.begin() + i);
                }
            });
            // [RENDER: remBtn, colour = style.statusError]
            remBtn.OnEvent(UI::WidgetEvent::None, 420, rowY);
            rowY += 20;
        }
        rowY += 6;
    }

    // ── Pack button ──────────────────────────────────────────────────────────
    {
        UI::Button packBtn("Pack");
        packBtn.SetBounds({0, rowY, 80, 24});
        packBtn.SetOnClick([this]() {
            if (!m_impl->packer || m_impl->pendingTextures.empty()) return;
            m_impl->lastResult = m_impl->packer->Pack(m_impl->config);
            m_impl->hasPacked  = true;
        });
        // [RENDER: packBtn, colour = style.accentPressed]
        packBtn.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 32;
    }

    // ── Pack result stats ────────────────────────────────────────────────────
    if (m_impl->hasPacked) {
        const AtlasStats& st = m_impl->lastResult.stats;
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1)
           << "Pages: " << st.pageCount
           << "  Packed: " << st.packedCount << "/" << st.inputCount
           << "  Avg util: " << (st.avgUtilisation * 100.f) << "%"
           << "  Time: " << st.packTimeMs << " ms";

        UI::Label statsLbl(ss.str());
        statsLbl.SetBounds({0, rowY, 700, 20});
        // [RENDER: statsLbl, colour = style.statusOk]
        statsLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
        rowY += 26;

        if (!m_impl->lastResult.unpacked.empty()) {
            UI::Label unpackHdr("Unpacked:");
            unpackHdr.SetBounds({0, rowY, 200, 20});
            // [RENDER: unpackHdr, colour = style.statusWarning]
            unpackHdr.OnEvent(UI::WidgetEvent::None, 0, rowY);
            rowY += 22;

            for (const auto& name : m_impl->lastResult.unpacked) {
                UI::Label uLbl(name);
                uLbl.SetBounds({0, rowY, 300, 18});
                // [RENDER: uLbl, colour = style.statusError]
                uLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);
                rowY += 20;
            }
        }

        // Per-page utilisation bars
        for (const auto& page : m_impl->lastResult.pages) {
            std::ostringstream ps;
            ps << "Page " << page.pageIndex << ": "
               << std::fixed << std::setprecision(1)
               << (page.utilisation * 100.f) << "%";

            UI::Label pLbl(ps.str());
            pLbl.SetBounds({0, rowY, 200, 18});
            // [RENDER: pLbl, colour = style.textPrimary]
            pLbl.OnEvent(UI::WidgetEvent::None, 0, rowY);

            UI::ProgressBar bar;
            bar.SetProgress(page.utilisation);
            bar.SetBounds({210, rowY, 200, 14});
            // [RENDER: bar, fill = style.accent]
            bar.OnEvent(UI::WidgetEvent::None, 210, rowY);
            rowY += 22;
        }
    }
}

} // namespace Tools
