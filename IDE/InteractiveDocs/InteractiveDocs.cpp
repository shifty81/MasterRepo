#include "IDE/InteractiveDocs/InteractiveDocs.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#  include <stdio.h>   // provides _popen / _pclose in global namespace under MSVC
#  define popen  _popen
#  define pclose _pclose
#else
#  include <cstdio>
#endif

namespace fs = std::filesystem;

namespace IDE {

// ── Singleton ──────────────────────────────────────────────────

InteractiveDocs& InteractiveDocs::Get() {
    static InteractiveDocs instance;
    return instance;
}

// ── Library ────────────────────────────────────────────────────

void InteractiveDocs::LoadFromDirectory(const std::string& docsDir) {
    if (!fs::exists(docsDir)) return;
    for (auto& entry : fs::recursive_directory_iterator(docsDir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".md") continue;

        DocPage page;
        page.filePath = entry.path().string();
        page.id       = entry.path().stem().string();
        page.title    = page.id;

        std::ifstream f(entry.path());
        if (!f.is_open()) continue;
        std::ostringstream oss;
        oss << f.rdbuf();
        page.content = oss.str();

        // Extract title from first # heading
        std::istringstream iss(page.content);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line[0] == '#') {
                size_t pos = line.find_first_not_of("# ");
                page.title = (pos != std::string::npos) ? line.substr(pos) : page.id;
                break;
            }
        }

        // Look for code blocks → add examples
        page.hasExamples = page.content.find("```") != std::string::npos;
        if (page.hasExamples) page.hasRunButton = true;

        AddPage(page);
    }
}

void InteractiveDocs::AddPage(const DocPage& page) {
    m_pages[page.id] = page;
}

void InteractiveDocs::AddExample(const DocExample& example) {
    m_examples[example.pageId].push_back(example);
}

void InteractiveDocs::ClearLibrary() {
    m_pages.clear();
    m_examples.clear();
    m_history.clear();
    m_historyPos = -1;
}

// ── Navigation ────────────────────────────────────────────────

void InteractiveDocs::OpenPage(const std::string& id) {
    auto it = m_pages.find(id);
    if (it == m_pages.end()) return;

    // Truncate forward history
    if (m_historyPos + 1 < static_cast<int>(m_history.size())) {
        m_history.erase(m_history.begin() + m_historyPos + 1, m_history.end());
    }
    m_history.push_back(id);
    m_historyPos = static_cast<int>(m_history.size()) - 1;

    if (m_onPageOpened) m_onPageOpened(it->second);
}

void InteractiveDocs::GoBack() {
    if (m_historyPos <= 0) return;
    --m_historyPos;
    OpenPage(m_history[static_cast<size_t>(m_historyPos)]);
}

void InteractiveDocs::GoForward() {
    if (m_historyPos + 1 >= static_cast<int>(m_history.size())) return;
    ++m_historyPos;
    OpenPage(m_history[static_cast<size_t>(m_historyPos)]);
}

void InteractiveDocs::GoHome() {
    if (!m_pages.empty()) OpenPage(m_pages.begin()->first);
}

// ── Search ────────────────────────────────────────────────────

std::vector<DocSearchResult> InteractiveDocs::Search(const std::string& query,
                                                       uint32_t maxResults) const {
    std::vector<DocSearchResult> results;
    for (auto& [id, page] : m_pages) {
        float r = Relevance(page, query);
        if (r > 0.f) {
            DocSearchResult sr;
            sr.pageId    = id;
            sr.title     = page.title;
            sr.relevance = r;
            // Extract a snippet around the first match
            auto pos = page.content.find(query);
            if (pos != std::string::npos) {
                size_t start = (pos > 60) ? pos - 60 : 0;
                size_t len   = std::min<size_t>(120, page.content.size() - start);
                sr.snippet   = page.content.substr(start, len);
            }
            results.push_back(std::move(sr));
        }
    }
    std::sort(results.begin(), results.end(),
              [](const DocSearchResult& a, const DocSearchResult& b){
                  return a.relevance > b.relevance;
              });
    if (results.size() > maxResults) results.resize(maxResults);
    return results;
}

DocSearchResult InteractiveDocs::ContextHelp(const std::string& symbol) const {
    auto results = Search(symbol, 1);
    if (!results.empty()) return results.front();
    DocSearchResult empty;
    empty.snippet = "No documentation found for '" + symbol + "'.";
    return empty;
}

// ── Run example ───────────────────────────────────────────────

std::string InteractiveDocs::RunExample(const std::string& pageId, size_t exampleIndex) {
    auto it = m_examples.find(pageId);
    if (it == m_examples.end() || exampleIndex >= it->second.size())
        return "No example found.";

    const DocExample& ex = it->second[exampleIndex];
    // Write to temp file and run (Lua / Python only for safety; C++ just displayed)
    if (ex.language == "lua") {
        namespace fs = std::filesystem;
        std::string tmp = (fs::temp_directory_path() / "doc_example.lua").string();
        std::ofstream f(tmp);
        if (f.is_open()) { f << ex.code; f.close(); }
        // NOTE: tmp path is generated by the system; the example code is treated as trusted
        // (loaded from the documentation library, not from user input).
        std::string luaCmd = "lua " + tmp + " 2>&1";
        FILE* pipe = popen(luaCmd.c_str(), "r");
        if (pipe) {
            std::string out;
            char buf[256];
            while (fgets(buf, sizeof(buf), pipe)) out += buf;
            pclose(pipe);
            return out;
        }
    }
    // For C++: just return the code itself
    return ex.code;
}

// ── Render stub ────────────────────────────────────────────────

void InteractiveDocs::Render() {
    // Rendered by IDE panel / UI::GUISystem in production
}

// ── Query ─────────────────────────────────────────────────────

const DocPage* InteractiveDocs::GetPage(const std::string& id) const {
    auto it = m_pages.find(id);
    return (it != m_pages.end()) ? &it->second : nullptr;
}

std::vector<const DocPage*> InteractiveDocs::GetByTag(const std::string& tag) const {
    std::vector<const DocPage*> out;
    for (auto& [id, page] : m_pages) {
        if (std::find(page.tags.begin(), page.tags.end(), tag) != page.tags.end()) {
            out.push_back(&page);
        }
    }
    return out;
}

std::vector<const DocPage*> InteractiveDocs::GetRelated(const std::string& pageId) const {
    std::vector<const DocPage*> out;
    auto it = m_pages.find(pageId);
    if (it == m_pages.end()) return out;
    for (auto& relId : it->second.relatedPageIds) {
        const DocPage* p = GetPage(relId);
        if (p) out.push_back(p);
    }
    return out;
}

// ── Callbacks ─────────────────────────────────────────────────

void InteractiveDocs::OnPageOpened(PageOpenedCallback cb) {
    m_onPageOpened = std::move(cb);
}

// ── Private helpers ────────────────────────────────────────────

float InteractiveDocs::Relevance(const DocPage& page, const std::string& query) const {
    if (query.empty()) return 0.f;
    float score = 0.f;
    // Title match is worth more
    if (page.title.find(query) != std::string::npos) score += 2.f;
    if (page.content.find(query) != std::string::npos) score += 1.f;
    for (auto& tag : page.tags) {
        if (tag.find(query) != std::string::npos) score += 0.5f;
    }
    return score;
}

} // namespace IDE
