#include "AI/MetaLearning/MetaLearning.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <sstream>

namespace fs = std::filesystem;
namespace AI {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static uint64_t NowSecs()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

/* static */
float MetaLearner::CosineSimilarity(const std::vector<float>& a,
                                     const std::vector<float>& b)
{
    if (a.empty() || b.empty() || a.size() != b.size()) return 0.f;
    float dot = 0.f, na = 0.f, nb = 0.f;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        na  += a[i] * a[i];
        nb  += b[i] * b[i];
    }
    float denom = std::sqrt(na) * std::sqrt(nb);
    return (denom > 1e-8f) ? (dot / denom) : 0.f;
}

/* static */
float MetaLearner::KeywordOverlap(const std::string& a, const std::string& b)
{
    // Convert to lowercase word bags and compute Jaccard overlap
    auto words = [](const std::string& s) {
        std::vector<std::string> w;
        std::istringstream iss(s);
        std::string tok;
        while (iss >> tok) {
            for (char& c : tok) c = (char)std::tolower((unsigned char)c);
            if (tok.size() >= 3) w.push_back(tok);
        }
        std::sort(w.begin(), w.end());
        w.erase(std::unique(w.begin(), w.end()), w.end());
        return w;
    };

    auto wa = words(a);
    auto wb = words(b);
    if (wa.empty() && wb.empty()) return 1.f;
    if (wa.empty() || wb.empty()) return 0.f;

    std::vector<std::string> inter;
    std::set_intersection(wa.begin(), wa.end(),
                          wb.begin(), wb.end(),
                          std::back_inserter(inter));

    std::vector<std::string> uni;
    std::set_union(wa.begin(), wa.end(),
                   wb.begin(), wb.end(),
                   std::back_inserter(uni));

    return (float)inter.size() / (float)uni.size();
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

MetaLearner::MetaLearner(MetaLearningConfig cfg)
    : m_cfg(std::move(cfg))
{
    Load();
}

MetaLearner::~MetaLearner()
{
    if (m_cfg.autoSave) Save();
}

// ─────────────────────────────────────────────────────────────────────────────
// Ingestion
// ─────────────────────────────────────────────────────────────────────────────

std::string MetaLearner::GenerateId() const
{
    // Simple timestamp + count ID (no UUID dependency)
    return "ml_" + std::to_string(NowSecs()) + "_" +
           std::to_string(m_entries.size());
}

void MetaLearner::RecordOutcome(const KnowledgeEntry& entry)
{
    KnowledgeEntry e = entry;
    if (e.id.empty())        e.id        = GenerateId();
    if (e.timestamp == 0)    e.timestamp = NowSecs();

    // Compute embedding if a function is available
    if (m_cfg.embedFn && e.embedding.empty())
        e.embedding = m_cfg.embedFn(e.task + " " + e.context);

    m_entries.push_back(std::move(e));

    // Evict oldest entries if over limit
    if ((int)m_entries.size() > m_cfg.maxEntries) {
        std::sort(m_entries.begin(), m_entries.end(),
                  [](const KnowledgeEntry& a, const KnowledgeEntry& b){
                      return a.timestamp < b.timestamp;
                  });
        m_entries.erase(m_entries.begin(),
                        m_entries.begin() + ((int)m_entries.size() - m_cfg.maxEntries));
    }
}

void MetaLearner::Import(const std::vector<KnowledgeEntry>& entries)
{
    for (const auto& e : entries) RecordOutcome(e);
}

// ─────────────────────────────────────────────────────────────────────────────
// Retrieval
// ─────────────────────────────────────────────────────────────────────────────

float MetaLearner::ComputeScore(const KnowledgeEntry& entry,
                                 const std::vector<float>& queryEmb,
                                 const std::string& queryText) const
{
    float score = 0.f;
    if (!queryEmb.empty() && !entry.embedding.empty())
        score = CosineSimilarity(queryEmb, entry.embedding);
    else
        score = KeywordOverlap(queryText, entry.task + " " + entry.context);

    // Boost by quality
    float qBoost = 1.0f;
    switch (entry.quality) {
        case OutcomeQuality::Excellent: qBoost = 1.5f; break;
        case OutcomeQuality::Good:      qBoost = 1.2f; break;
        case OutcomeQuality::Partial:   qBoost = 0.8f; break;
        case OutcomeQuality::Failed:    qBoost = 0.3f; break;
    }

    return score * qBoost * entry.successRate;
}

std::vector<QueryResult> MetaLearner::Query(const std::string& task,
                                             int topK) const
{
    if (topK < 0) topK = m_cfg.topK;

    std::vector<float> queryEmb;
    if (m_cfg.embedFn) queryEmb = m_cfg.embedFn(task);

    std::vector<QueryResult> scored;
    scored.reserve(m_entries.size());
    for (const auto& e : m_entries) {
        float s = ComputeScore(e, queryEmb, task);
        if (s >= m_cfg.minScore)
            scored.push_back({&e, s});
    }

    std::sort(scored.begin(), scored.end(),
              [](const QueryResult& a, const QueryResult& b){
                  return a.score > b.score;
              });

    if ((int)scored.size() > topK) scored.resize(topK);
    return scored;
}

std::vector<QueryResult> MetaLearner::QueryForAgent(const std::string& agentType,
                                                     const std::string& task,
                                                     int topK) const
{
    if (topK < 0) topK = m_cfg.topK;

    std::vector<float> queryEmb;
    if (m_cfg.embedFn) queryEmb = m_cfg.embedFn(task);

    std::vector<QueryResult> scored;
    for (const auto& e : m_entries) {
        if (e.agentType != agentType) continue;
        float s = ComputeScore(e, queryEmb, task);
        if (s >= m_cfg.minScore)
            scored.push_back({&e, s});
    }

    std::sort(scored.begin(), scored.end(),
              [](const QueryResult& a, const QueryResult& b){
                  return a.score > b.score;
              });

    if ((int)scored.size() > topK) scored.resize(topK);
    return scored;
}

// ─────────────────────────────────────────────────────────────────────────────
// Management
// ─────────────────────────────────────────────────────────────────────────────

size_t MetaLearner::EntryCount() const { return m_entries.size(); }

void MetaLearner::Clear()  { m_entries.clear(); }

int MetaLearner::Prune(OutcomeQuality minQuality)
{
    int before = (int)m_entries.size();
    m_entries.erase(
        std::remove_if(m_entries.begin(), m_entries.end(),
                       [minQuality](const KnowledgeEntry& e){
                           return e.quality < minQuality;
                       }),
        m_entries.end());
    return before - (int)m_entries.size();
}

void MetaLearner::RebuildEmbeddings()
{
    if (!m_cfg.embedFn) return;
    for (auto& e : m_entries)
        e.embedding = m_cfg.embedFn(e.task + " " + e.context);
}

// ─────────────────────────────────────────────────────────────────────────────
// Persistence (simple JSON-line format)
// ─────────────────────────────────────────────────────────────────────────────

static std::string Escape(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '"')  { out += "\\\""; }
        else if (c == '\\') { out += "\\\\"; }
        else if (c == '\n') { out += "\\n"; }
        else if (c == '\r') { out += "\\r"; }
        else                { out += c; }
    }
    return out;
}

static std::string Unescape(const std::string& s)
{
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            ++i;
            switch (s[i]) {
                case '"':  out += '"';  break;
                case '\\': out += '\\'; break;
                case 'n':  out += '\n'; break;
                case 'r':  out += '\r'; break;
                default:   out += '\\'; out += s[i]; break;
            }
        } else {
            out += s[i];
        }
    }
    return out;
}

bool MetaLearner::Save() const
{
    try {
        fs::create_directories(m_cfg.storePath);
        std::ofstream ofs(m_cfg.storePath + "/entries.jsonl");
        if (!ofs) return false;

        for (const auto& e : m_entries) {
            ofs << "{"
                << "\"id\":\"" << Escape(e.id) << "\","
                << "\"agent\":\"" << Escape(e.agentType) << "\","
                << "\"task\":\"" << Escape(e.task) << "\","
                << "\"snippet\":\"" << Escape(e.solutionSnippet) << "\","
                << "\"quality\":" << (int)e.quality << ","
                << "\"useCount\":" << e.useCount << ","
                << "\"successRate\":" << e.successRate << ","
                << "\"ts\":" << e.timestamp
                << "}\n";
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool MetaLearner::Load()
{
    std::ifstream ifs(m_cfg.storePath + "/entries.jsonl");
    if (!ifs) return false;

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty() || line[0] != '{') continue;

        // Minimal field extraction
        auto extract = [&](const std::string& key) -> std::string {
            std::string needle = "\"" + key + "\":\"";
            auto pos = line.find(needle);
            if (pos == std::string::npos) return {};
            pos += needle.size();
            auto end = line.find('"', pos);
            // handle escaped quotes
            while (end != std::string::npos && line[end-1] == '\\') {
                end = line.find('"', end + 1);
            }
            if (end == std::string::npos) return {};
            return Unescape(line.substr(pos, end - pos));
        };
        auto extractInt = [&](const std::string& key) -> int {
            std::string needle = "\"" + key + "\":";
            auto pos = line.find(needle);
            if (pos == std::string::npos) return 0;
            pos += needle.size();
            auto end = line.find_first_of(",}", pos);
            try { return std::stoi(line.substr(pos, end - pos)); } catch (...) { return 0; }
        };
        auto extractFloat = [&](const std::string& key) -> float {
            std::string needle = "\"" + key + "\":";
            auto pos = line.find(needle);
            if (pos == std::string::npos) return 0.f;
            pos += needle.size();
            auto end = line.find_first_of(",}", pos);
            try { return std::stof(line.substr(pos, end - pos)); } catch (...) { return 0.f; }
        };

        KnowledgeEntry e;
        e.id              = extract("id");
        e.agentType       = extract("agent");
        e.task            = extract("task");
        e.solutionSnippet = extract("snippet");
        e.quality         = static_cast<OutcomeQuality>(extractInt("quality"));
        e.useCount        = extractInt("useCount");
        e.successRate     = extractFloat("successRate");
        e.timestamp       = static_cast<uint64_t>(extractInt("ts"));

        if (!e.id.empty()) m_entries.push_back(std::move(e));
    }
    return !m_entries.empty();
}

// ─────────────────────────────────────────────────────────────────────────────
// Stats
// ─────────────────────────────────────────────────────────────────────────────

MetaLearner::Stats MetaLearner::GetStats() const
{
    Stats s;
    s.totalEntries = m_entries.size();
    float sumSuccess = 0.f;
    for (const auto& e : m_entries) {
        switch (e.quality) {
            case OutcomeQuality::Excellent: ++s.excellentCount; break;
            case OutcomeQuality::Good:      ++s.goodCount;      break;
            case OutcomeQuality::Partial:   ++s.partialCount;   break;
            case OutcomeQuality::Failed:    ++s.failedCount;    break;
        }
        s.byAgentType[e.agentType]++;
        sumSuccess += e.successRate;
    }
    s.avgSuccessRate = m_entries.empty() ? 0.f :
                       sumSuccess / (float)m_entries.size();
    return s;
}

} // namespace AI
