#include "AI/Training/TrainingPipeline.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <unordered_set>

namespace AI {

static uint64_t TPNowMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

// Simple JSON string escaping (no external library dependency)
static std::string JsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:   out += c;      break;
        }
    }
    return out;
}

static std::string SourceName(SampleSource s) {
    switch (s) {
    case SampleSource::Archive:        return "archive";
    case SampleSource::AssetIndex:     return "asset_index";
    case SampleSource::ErrorFix:       return "error_fix";
    case SampleSource::CodeGen:        return "code_gen";
    case SampleSource::UserCorrection: return "user_correction";
    }
    return "unknown";
}

static SampleSource SourceFromName(const std::string& s) {
    if (s == "asset_index")     return SampleSource::AssetIndex;
    if (s == "error_fix")       return SampleSource::ErrorFix;
    if (s == "code_gen")        return SampleSource::CodeGen;
    if (s == "user_correction") return SampleSource::UserCorrection;
    return SampleSource::Archive;
}

// ─────────────────────────────────────────────────────────────────────────────
// Collection
// ─────────────────────────────────────────────────────────────────────────────

void TrainingPipeline::AddSample(TrainingSample s) {
    if (s.timestamp == 0) s.timestamp = TPNowMs();
    m_samples.push_back(std::move(s));
}

void TrainingPipeline::CollectFromArchiveLearning(const std::string& indexPath) {
    // Each line in the archive index: REC\tpath\trepo\tlang\tts
    // We build a prompt/completion pair from path+repo → content
    std::ifstream f(indexPath);
    if (!f) return;

    std::string line;
    TrainingSample cur;
    bool inContent = false;
    size_t contentLeft = 0;

    while (std::getline(f, line)) {
        if (contentLeft > 0) {
            cur.completion += line + "\n";
            contentLeft -= std::min(contentLeft, line.size() + 1);
            if (contentLeft == 0 && !cur.prompt.empty()) {
                cur.source    = SampleSource::Archive;
                cur.timestamp = TPNowMs();
                m_samples.push_back(cur);
                cur = {};
            }
            continue;
        }
        if (line.substr(0, 4) == "REC\t") {
            cur = {};
            std::istringstream ss(line.substr(4));
            std::string path, repo, lang, ts;
            std::getline(ss, path, '\t');
            std::getline(ss, repo, '\t');
            std::getline(ss, lang, '\t');
            std::getline(ss, ts,   '\t');
            cur.prompt = "// File: " + path + "\n// Repo: " + repo + "\n// Language: " + lang;
            cur.tags   = {lang};
        } else if (line.substr(0, 8) == "CONTENT\t") {
            try { contentLeft = std::stoull(line.substr(8)); } catch (...) {}
        }
    }
}

void TrainingPipeline::CollectFromErrorFixes(const std::string& knowledgePath) {
    // Tab-separated: pattern \t fix \t occurrences \t successRate
    std::ifstream f(knowledgePath);
    if (!f) return;

    std::string line;
    while (std::getline(f, line)) {
        size_t t1 = line.find('\t');
        if (t1 == std::string::npos) continue;
        size_t t2 = line.find('\t', t1 + 1);
        if (t2 == std::string::npos) continue;

        std::string pattern = line.substr(0, t1);
        std::string fix     = line.substr(t1 + 1, t2 - t1 - 1);
        if (pattern.empty() || fix.empty()) continue;

        float successRate = 0.0f;
        size_t t3 = line.find('\t', t2 + 1);
        if (t3 != std::string::npos) {
            try { successRate = std::stof(line.substr(t3 + 1)); } catch (...) {}
        }

        TrainingSample s;
        s.prompt     = "Fix this compiler error: " + pattern;
        s.completion = fix;
        s.source     = SampleSource::ErrorFix;
        s.quality    = successRate;
        s.tags       = {"error_fix"};
        s.timestamp  = TPNowMs();
        m_samples.push_back(std::move(s));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Filtering
// ─────────────────────────────────────────────────────────────────────────────

void TrainingPipeline::FilterByQuality(float minQuality) {
    m_samples.erase(
        std::remove_if(m_samples.begin(), m_samples.end(),
                       [minQuality](const TrainingSample& s){ return s.quality < minQuality; }),
        m_samples.end());
}

void TrainingPipeline::FilterBySource(SampleSource src) {
    m_samples.erase(
        std::remove_if(m_samples.begin(), m_samples.end(),
                       [src](const TrainingSample& s){ return s.source != src; }),
        m_samples.end());
}

void TrainingPipeline::DeduplicateByPrompt() {
    std::unordered_set<std::string> seen;
    std::vector<TrainingSample> unique;
    unique.reserve(m_samples.size());
    for (auto& s : m_samples) {
        if (seen.insert(s.prompt).second)
            unique.push_back(std::move(s));
    }
    m_samples = std::move(unique);
}

// ─────────────────────────────────────────────────────────────────────────────
// Export
// ─────────────────────────────────────────────────────────────────────────────

bool TrainingPipeline::ExportJSONL(const std::string& outputPath) const {
    std::ofstream f(outputPath);
    if (!f) return false;
    for (const auto& s : m_samples) {
        f << "{\"prompt\":\"" << JsonEscape(s.prompt)
          << "\",\"completion\":\"" << JsonEscape(s.completion)
          << "\",\"source\":\"" << SourceName(s.source)
          << "\",\"quality\":" << s.quality
          << "}\n";
    }
    m_totalExported += m_samples.size();
    return true;
}

bool TrainingPipeline::ExportAlpaca(const std::string& outputPath) const {
    std::ofstream f(outputPath);
    if (!f) return false;
    f << "[\n";
    for (size_t i = 0; i < m_samples.size(); ++i) {
        const auto& s = m_samples[i];
        f << "  {\"instruction\":\"" << JsonEscape(s.prompt)
          << "\",\"input\":\"\",\"output\":\"" << JsonEscape(s.completion) << "\"}";
        if (i + 1 < m_samples.size()) f << ",";
        f << "\n";
    }
    f << "]\n";
    m_totalExported += m_samples.size();
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Stats & accessors
// ─────────────────────────────────────────────────────────────────────────────

PipelineStats TrainingPipeline::GetStats() const {
    PipelineStats st;
    st.totalCollected = m_samples.size();
    st.afterFilter    = m_samples.size();
    st.totalExported  = m_totalExported;
    if (!m_samples.empty()) {
        float sum = 0.0f;
        for (const auto& s : m_samples) sum += s.quality;
        st.avgQuality = sum / static_cast<float>(m_samples.size());
    }
    return st;
}

size_t TrainingPipeline::SampleCount() const { return m_samples.size(); }
void   TrainingPipeline::Clear()             { m_samples.clear(); m_totalExported = 0; }
const std::vector<TrainingSample>& TrainingPipeline::Samples() const { return m_samples; }

} // namespace AI
