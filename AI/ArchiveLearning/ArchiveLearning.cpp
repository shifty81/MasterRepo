#include "AI/ArchiveLearning/ArchiveLearning.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace AI {

static uint64_t ArchNowMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string ArchiveLearning::DetectLanguage(const std::string& path) const {
    namespace fs = std::filesystem;
    std::string ext = fs::path(path).extension().string();
    if (ext == ".cpp" || ext == ".cxx" || ext == ".cc") return "cpp";
    if (ext == ".h"   || ext == ".hpp" || ext == ".hxx") return "cpp";
    if (ext == ".c")   return "c";
    if (ext == ".py")  return "python";
    if (ext == ".lua") return "lua";
    if (ext == ".js" || ext == ".ts") return "javascript";
    if (ext == ".cs")  return "csharp";
    if (ext == ".rs")  return "rust";
    if (ext == ".go")  return "go";
    return "unknown";
}

std::vector<std::string> ArchiveLearning::AutoTag(const std::string& content,
                                                    const std::string& lang) const {
    std::vector<std::string> tags;
    if (!lang.empty() && lang != "unknown") tags.push_back(lang);

    // Keyword → tag heuristics
    static const std::vector<std::pair<std::string, std::string>> kKeywords = {
        {"class ", "oop"}, {"struct ", "oop"},
        {"render", "render"}, {"Render", "render"},
        {"Component", "ecs"}, {"Entity", "ecs"}, {"System", "ecs"},
        {"UI", "ui"}, {"Widget", "ui"}, {"Panel", "ui"},
        {"Shader", "shader"}, {"shader", "shader"},
        {"Physics", "physics"}, {"Collider", "physics"},
        {"Audio", "audio"}, {"Sound", "audio"},
        {"AI::", "ai"}, {"neural", "ai"},
        {"Inventory", "inventory"}, {"Crafting", "crafting"},
        {"Network", "network"}, {"Socket", "network"},
    };

    for (const auto& [kw, tag] : kKeywords) {
        if (content.find(kw) != std::string::npos) {
            if (std::find(tags.begin(), tags.end(), tag) == tags.end())
                tags.push_back(tag);
        }
    }
    return tags;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void ArchiveLearning::ScanArchiveDir(const std::string& dir) {
    namespace fs = std::filesystem;
    if (!fs::exists(dir)) return;

    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string path = entry.path().string();

        std::ifstream f(path);
        if (!f) continue;
        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());

        ArchiveRecord rec;
        rec.path        = fs::relative(entry.path(), dir).string();
        rec.language    = DetectLanguage(path);
        rec.content     = std::move(content);
        rec.tags        = AutoTag(rec.content, rec.language);
        rec.indexedAt   = ArchNowMs();

        // Derive originalRepo from first path component above dir
        auto rel = fs::relative(entry.path(), fs::path(dir).parent_path());
        auto it = rel.begin();
        if (it != rel.end()) rec.originalRepo = it->string();

        IndexRecord(std::move(rec));
    }
}

void ArchiveLearning::IndexRecord(ArchiveRecord rec) {
    if (rec.indexedAt == 0) rec.indexedAt = ArchNowMs();
    if (rec.language.empty()) rec.language = DetectLanguage(rec.path);
    if (rec.tags.empty())     rec.tags     = AutoTag(rec.content, rec.language);
    m_records.push_back(std::move(rec));
}

void ArchiveLearning::ExtractPatterns() {
    // Count keyword frequencies across all records
    std::unordered_map<std::string, uint32_t> freq;
    std::unordered_map<std::string, std::string> snippet;

    static const std::vector<std::string> kCategories = {
        "ecs_component", "render_pass", "ui_widget", "shader_code",
        "physics_body", "audio_source", "ai_agent", "crafting_recipe",
    };
    static const std::vector<std::string> kCatKeywords = {
        "Component",   "RenderPass",  "Widget",      "Shader",
        "RigidBody",   "AudioSource", "AIAgent",     "CraftingRecipe",
    };

    for (const auto& rec : m_records) {
        for (size_t i = 0; i < kCategories.size(); ++i) {
            if (rec.content.find(kCatKeywords[i]) != std::string::npos) {
                freq[kCategories[i]]++;
                if (snippet.find(kCategories[i]) == snippet.end()) {
                    // Store a small representative snippet
                    auto pos = rec.content.find(kCatKeywords[i]);
                    auto start = (pos > 80) ? pos - 80 : 0;
                    snippet[kCategories[i]] = rec.content.substr(start,
                        std::min<size_t>(160, rec.content.size() - start));
                }
            }
        }
    }

    m_patterns.clear();
    for (const auto& [cat, count] : freq) {
        ArchivePattern p;
        p.category    = cat;
        p.frequency   = count;
        p.codeSnippet = snippet[cat];
        p.quality     = std::min(1.0f, count / 10.0f);
        m_patterns.push_back(p);
    }
    std::sort(m_patterns.begin(), m_patterns.end(),
              [](const ArchivePattern& a, const ArchivePattern& b){
                  return a.frequency > b.frequency;
              });
}

std::vector<ArchiveRecord> ArchiveLearning::Search(const std::string& query) const {
    std::vector<ArchiveRecord> results;
    for (const auto& rec : m_records) {
        if (rec.path.find(query)    != std::string::npos ||
            rec.content.find(query) != std::string::npos ||
            rec.originalRepo.find(query) != std::string::npos) {
            results.push_back(rec);
        }
    }
    return results;
}

std::vector<ArchiveRecord> ArchiveLearning::ByTag(const std::string& tag) const {
    std::vector<ArchiveRecord> results;
    for (const auto& rec : m_records) {
        for (const auto& t : rec.tags) {
            if (t == tag) { results.push_back(rec); break; }
        }
    }
    return results;
}

std::vector<ArchivePattern> ArchiveLearning::GetPatterns() const {
    return m_patterns;
}

std::vector<ArchivePattern> ArchiveLearning::GetPatternsByCategory(const std::string& cat) const {
    std::vector<ArchivePattern> results;
    for (const auto& p : m_patterns) {
        if (p.category == cat) results.push_back(p);
    }
    return results;
}

std::string ArchiveLearning::GenerateTrainingSample(const ArchiveRecord& rec) const {
    return "// File: " + rec.path + "\n// Repo: " + rec.originalRepo + "\n" + rec.content + "\n";
}

bool ArchiveLearning::SaveIndex(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return false;
    for (const auto& rec : m_records) {
        f << "REC\t" << rec.path << "\t" << rec.originalRepo << "\t"
          << rec.language << "\t" << rec.indexedAt << "\n";
        for (const auto& tag : rec.tags)
            f << "TAG\t" << tag << "\n";
        // Store content length so we can skip it during index-only reads
        f << "CONTENT\t" << rec.content.size() << "\n";
        f << rec.content << "\n";
    }
    return true;
}

bool ArchiveLearning::LoadIndex(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    m_records.clear();
    std::string line;
    ArchiveRecord* cur = nullptr;
    size_t pendingContent = 0;

    while (std::getline(f, line)) {
        if (pendingContent > 0) {
            if (cur) cur->content += line + "\n";
            pendingContent -= std::min(pendingContent, line.size() + 1);
            continue;
        }
        if (line.substr(0, 4) == "REC\t") {
            m_records.push_back({});
            cur = &m_records.back();
            // parse path \t repo \t lang \t ts
            std::istringstream ss(line.substr(4));
            std::getline(ss, cur->path,        '\t');
            std::getline(ss, cur->originalRepo, '\t');
            std::getline(ss, cur->language,     '\t');
            std::string ts; std::getline(ss, ts, '\t');
            try { cur->indexedAt = std::stoull(ts); } catch (...) {}
        } else if (line.substr(0, 4) == "TAG\t" && cur) {
            cur->tags.push_back(line.substr(4));
        } else if (line.substr(0, 8) == "CONTENT\t") {
            try { pendingContent = std::stoull(line.substr(8)); } catch (...) {}
        }
    }
    return true;
}

size_t ArchiveLearning::RecordCount()  const { return m_records.size(); }
size_t ArchiveLearning::PatternCount() const { return m_patterns.size(); }
void   ArchiveLearning::Clear()              { m_records.clear(); m_patterns.clear(); }

} // namespace AI
