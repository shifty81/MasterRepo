#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace IDE {

// ──────────────────────────────────────────────────────────────
// Doc page
// ──────────────────────────────────────────────────────────────

struct DocPage {
    std::string id;
    std::string title;
    std::string content;      // Markdown
    std::string filePath;     // source .md file
    std::vector<std::string> tags;
    std::vector<std::string> relatedPageIds;
    bool        hasExamples   = false;
    bool        hasRunButton  = false;
};

// ──────────────────────────────────────────────────────────────
// Code example that can be run in-editor
// ──────────────────────────────────────────────────────────────

struct DocExample {
    std::string pageId;
    std::string title;
    std::string code;         // C++ or Lua snippet
    std::string language;     // "cpp" | "lua" | "python"
    std::string expectedOutput;
};

// ──────────────────────────────────────────────────────────────
// Search result
// ──────────────────────────────────────────────────────────────

struct DocSearchResult {
    std::string pageId;
    std::string title;
    float       relevance = 0.f;
    std::string snippet;
};

// ──────────────────────────────────────────────────────────────
// InteractiveDocs — live documentation browser + runnable examples
// ──────────────────────────────────────────────────────────────

class InteractiveDocs {
public:
    // Library management
    void LoadFromDirectory(const std::string& docsDir);
    void AddPage(const DocPage& page);
    void AddExample(const DocExample& example);
    void ClearLibrary();

    // Navigation
    void OpenPage(const std::string& id);
    void GoBack();
    void GoForward();
    void GoHome();

    // Search
    std::vector<DocSearchResult> Search(const std::string& query,
                                        uint32_t maxResults = 20) const;

    // Context help: find the most relevant page for a symbol or keyword
    DocSearchResult ContextHelp(const std::string& symbol) const;

    // Run a code example in the IDE sandbox
    std::string RunExample(const std::string& pageId, size_t exampleIndex = 0);

    // Render UI (draws the documentation panel)
    void Render();

    // Query
    const DocPage*              GetPage(const std::string& id) const;
    std::vector<const DocPage*> GetByTag(const std::string& tag) const;
    std::vector<const DocPage*> GetRelated(const std::string& pageId) const;
    size_t PageCount() const { return m_pages.size(); }

    // Callbacks
    using PageOpenedCallback = std::function<void(const DocPage&)>;
    void OnPageOpened(PageOpenedCallback cb);

    // Singleton
    static InteractiveDocs& Get();

private:
    float Relevance(const DocPage& page, const std::string& query) const;

    std::unordered_map<std::string, DocPage>          m_pages;
    std::unordered_map<std::string, std::vector<DocExample>> m_examples;
    std::vector<std::string>                          m_history;
    int                                               m_historyPos = -1;
    PageOpenedCallback                                m_onPageOpened;
};

} // namespace IDE
