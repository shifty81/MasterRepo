#include "Tools/Packing/TextureAtlasPacker/TextureAtlasPacker.h"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <stdexcept>

namespace Tools {

// ── Input management ──────────────────────────────────────────────────────
void TextureAtlasPacker::AddTexture(const TextureEntry& entry) {
    m_textures.push_back(entry);
}
bool TextureAtlasPacker::RemoveTexture(const std::string& name) {
    auto it = std::find_if(m_textures.begin(), m_textures.end(),
        [&name](const TextureEntry& e){ return e.name == name; });
    if (it == m_textures.end()) return false;
    m_textures.erase(it);
    return true;
}
void   TextureAtlasPacker::ClearTextures()  { m_textures.clear(); }
size_t TextureAtlasPacker::TextureCount() const { return m_textures.size(); }

// ── Page utilisation ──────────────────────────────────────────────────────
float TextureAtlasPacker::PageUtil(const AtlasPage& page, uint32_t pw, uint32_t ph) {
    uint64_t used = 0;
    for (const auto& pt : page.packed) used += uint64_t(pt.width) * pt.height;
    uint64_t total = uint64_t(pw) * ph;
    return total > 0 ? static_cast<float>(used) / static_cast<float>(total) : 0.0f;
}

// ── Shelf packing ─────────────────────────────────────────────────────────
PackResult TextureAtlasPacker::PackShelf(
    const AtlasConfig& cfg, std::vector<TextureEntry> sorted) const {

    PackResult result;
    if (sorted.empty()) { result.success = true; return result; }

    AtlasPage  curPage;
    curPage.pageIndex = 0;
    uint32_t shelfY   = 0;
    uint32_t shelfH   = 0;
    uint32_t cursorX  = 0;

    auto commitPage = [&]() {
        curPage.utilisation = PageUtil(curPage, cfg.pageWidth, cfg.pageHeight);
        result.pages.push_back(curPage);
    };
    auto newPage = [&]() {
        commitPage();
        curPage       = AtlasPage{};
        curPage.pageIndex = static_cast<uint32_t>(result.pages.size());
        shelfY = shelfH = cursorX = 0;
    };

    for (auto& te : sorted) {
        uint32_t w = te.width  + cfg.padding;
        uint32_t h = te.height + cfg.padding;

        if (w > cfg.pageWidth || h > cfg.pageHeight) {
            result.unpacked.push_back(te.name);
            continue;
        }
        // Try to fit on current shelf
        if (cursorX + w > cfg.pageWidth) {
            // New shelf
            shelfY  += shelfH;
            shelfH   = 0;
            cursorX  = 0;
        }
        if (shelfY + h > cfg.pageHeight) {
            // New page — guard before committing current page
            if (static_cast<uint32_t>(result.pages.size()) + 1 >= cfg.maxPages) {
                result.unpacked.push_back(te.name);
                continue;
            }
            newPage();
        }
        PackedTexture pt;
        pt.name      = te.name;
        pt.pageIndex = curPage.pageIndex;
        pt.x         = cursorX;
        pt.y         = shelfY;
        pt.width     = te.width;
        pt.height    = te.height;
        pt.rotated   = false;
        curPage.packed.push_back(pt);
        cursorX += w;
        shelfH   = std::max(shelfH, h);
    }
    commitPage();
    result.success = true;
    return result;
}

// ── MaxRects packing (simplified guillotine split) ────────────────────────
PackResult TextureAtlasPacker::PackMaxRects(
    const AtlasConfig& cfg, std::vector<TextureEntry> sorted) const {

    struct Rect { uint32_t x, y, w, h; };

    PackResult result;
    if (sorted.empty()) { result.success = true; return result; }

    std::vector<Rect> freeRects;
    uint32_t pageIdx = 0;
    AtlasPage curPage;
    curPage.pageIndex = 0;

    auto resetFree = [&]() {
        freeRects.clear();
        freeRects.push_back({0, 0, cfg.pageWidth, cfg.pageHeight});
    };
    resetFree();

    auto commitPage = [&]() {
        curPage.utilisation = PageUtil(curPage, cfg.pageWidth, cfg.pageHeight);
        result.pages.push_back(curPage);
    };
    auto newPage = [&]() {
        commitPage();
        ++pageIdx;
        curPage = AtlasPage{};
        curPage.pageIndex = pageIdx;
        resetFree();
    };

    for (auto& te : sorted) {
        uint32_t w = te.width  + cfg.padding;
        uint32_t h = te.height + cfg.padding;
        bool placed = false;

        // Try original, then rotated
        for (int rot = 0; rot < (cfg.allowRotation ? 2 : 1) && !placed; ++rot) {
            uint32_t pw = (rot == 0) ? w : h;
            uint32_t ph = (rot == 0) ? h : w;
            if (pw > cfg.pageWidth || ph > cfg.pageHeight) continue;

            // Find best-area-fit free rect
            int bestIdx = -1;
            uint64_t bestArea = UINT64_MAX;
            for (int i = 0; i < static_cast<int>(freeRects.size()); ++i) {
                const auto& fr = freeRects[i];
                if (fr.w >= pw && fr.h >= ph) {
                    uint64_t waste = uint64_t(fr.w)*fr.h - uint64_t(pw)*ph;
                    if (waste < bestArea) { bestArea = waste; bestIdx = i; }
                }
            }
            if (bestIdx < 0) continue;

            // Place
            const Rect fr = freeRects[bestIdx];
            PackedTexture pt;
            pt.name      = te.name;
            pt.pageIndex = curPage.pageIndex;
            pt.x         = fr.x;
            pt.y         = fr.y;
            pt.width     = te.width;
            pt.height    = te.height;
            pt.rotated   = (rot == 1);
            curPage.packed.push_back(pt);
            placed = true;

            // Split remaining space (guillotine)
            freeRects.erase(freeRects.begin() + bestIdx);
            if (fr.w - pw > 0) freeRects.push_back({fr.x + pw, fr.y, fr.w - pw, fr.h});
            if (fr.h - ph > 0) freeRects.push_back({fr.x, fr.y + ph, pw, fr.h - ph});
        }

        if (!placed) {
            // Guard before committing current page and opening a new one
            if (static_cast<uint32_t>(result.pages.size()) + 1 >= cfg.maxPages) {
                result.unpacked.push_back(te.name);
                continue;
            }
            newPage();
            // Retry on fresh page
            uint32_t pw = w, ph = h;
            if (pw <= cfg.pageWidth && ph <= cfg.pageHeight && !freeRects.empty()) {
                const Rect& fr = freeRects[0];
                PackedTexture pt;
                pt.name      = te.name;
                pt.pageIndex = curPage.pageIndex;
                pt.x = fr.x; pt.y = fr.y;
                pt.width = te.width; pt.height = te.height;
                pt.rotated = false;
                curPage.packed.push_back(pt);
                freeRects.erase(freeRects.begin());
                if (fr.w - pw > 0) freeRects.push_back({fr.x+pw, fr.y, fr.w-pw, fr.h});
                if (fr.h - ph > 0) freeRects.push_back({fr.x, fr.y+ph, pw, fr.h-ph});
            } else {
                result.unpacked.push_back(te.name);
            }
        }
    }
    commitPage();
    result.success = true;
    return result;
}

// ── Pack ──────────────────────────────────────────────────────────────────
PackResult TextureAtlasPacker::Pack(const AtlasConfig& config) {
    using Clock = std::chrono::steady_clock;
    auto t0 = Clock::now();

    // Sort by area descending (improves packing efficiency)
    std::vector<TextureEntry> sorted = m_textures;
    std::stable_sort(sorted.begin(), sorted.end(),
        [](const TextureEntry& a, const TextureEntry& b){
            return uint64_t(a.width)*a.height > uint64_t(b.width)*b.height;
        });

    PackResult result = (config.algo == PackAlgo::MaxRects)
        ? PackMaxRects(config, sorted)
        : PackShelf   (config, sorted);

    auto t1 = Clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    AtlasStats& st = result.stats;
    st.inputCount    = static_cast<uint32_t>(m_textures.size());
    st.unpackedCount = static_cast<uint32_t>(result.unpacked.size());
    st.packedCount   = st.inputCount - st.unpackedCount;
    st.pageCount     = static_cast<uint32_t>(result.pages.size());
    float totalUtil  = 0.0f;
    for (const auto& p : result.pages) totalUtil += p.utilisation;
    st.avgUtilisation = st.pageCount > 0 ? totalUtil / static_cast<float>(st.pageCount) : 0.0f;
    st.packTimeMs    = ms;

    m_lastResult = result;
    return result;
}

// ── Result queries ────────────────────────────────────────────────────────
const PackedTexture* TextureAtlasPacker::GetPackedTexture(const std::string& name) const {
    for (const auto& page : m_lastResult.pages) {
        for (const auto& pt : page.packed) {
            if (pt.name == name) return &pt;
        }
    }
    return nullptr;
}

// ── JSON export ───────────────────────────────────────────────────────────
std::string TextureAtlasPacker::ExportManifestJSON(const PackResult& result) const {
    std::ostringstream ss;
    ss << "{\n  \"pages\": [\n";
    for (size_t pi = 0; pi < result.pages.size(); ++pi) {
        const auto& page = result.pages[pi];
        ss << "    { \"page\": " << page.pageIndex
           << ", \"utilisation\": " << page.utilisation
           << ", \"sprites\": [\n";
        for (size_t si = 0; si < page.packed.size(); ++si) {
            const auto& pt = page.packed[si];
            ss << "      { \"name\": \"" << pt.name << "\""
               << ", \"x\": " << pt.x << ", \"y\": " << pt.y
               << ", \"w\": " << pt.width << ", \"h\": " << pt.height
               << ", \"rotated\": " << (pt.rotated ? "true" : "false") << " }";
            if (si + 1 < page.packed.size()) ss << ",";
            ss << "\n";
        }
        ss << "    ]}";
        if (pi + 1 < result.pages.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";
    ss << "  \"unpacked\": [";
    for (size_t i = 0; i < result.unpacked.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << "\"" << result.unpacked[i] << "\"";
    }
    ss << "]\n}\n";
    return ss.str();
}

// ── Stats ─────────────────────────────────────────────────────────────────
AtlasStats TextureAtlasPacker::GetLastStats() const { return m_lastResult.stats; }

} // namespace Tools
