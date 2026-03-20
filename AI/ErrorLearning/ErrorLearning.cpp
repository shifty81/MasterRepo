#include "AI/ErrorLearning/ErrorLearning.h"
#include <algorithm>
#include <chrono>
#include <fstream>

namespace AI {

static uint64_t ErrNowMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

void ErrorLearning::RecordError(const BuildError& error) {
    BuildError e = error;
    if (e.timestamp == 0) e.timestamp = ErrNowMs();
    m_errorHistory.push_back(e);
    findOrCreatePattern(e.errorCode)->occurrences++;
}

void ErrorLearning::RecordFix(const std::string& errorCode, const std::string& fix, bool success) {
    auto* p = findOrCreatePattern(errorCode);
    p->suggestedFix = fix;
    float total = static_cast<float>(p->occurrences);
    if (total > 0)
        p->successRate = (p->successRate * (total - 1) + (success ? 1.0f : 0.0f)) / total;
    else
        p->successRate = success ? 1.0f : 0.0f;
}

std::string ErrorLearning::SuggestFix(const std::string& errorMessage) const {
    const ErrorPattern* best = nullptr;
    size_t bestLen = 0;
    for (const auto& p : m_patterns) {
        if (!p.pattern.empty() && errorMessage.find(p.pattern) != std::string::npos) {
            if (p.pattern.size() > bestLen) {
                bestLen = p.pattern.size();
                best = &p;
            }
        }
    }
    if (best) return best->suggestedFix;
    return "";
}

const std::vector<ErrorPattern>& ErrorLearning::GetPatterns() const { return m_patterns; }
const std::vector<BuildError>&   ErrorLearning::GetErrorHistory() const { return m_errorHistory; }

bool ErrorLearning::SaveKnowledge(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return false;
    for (const auto& p : m_patterns) {
        f << p.pattern << "\t" << p.suggestedFix << "\t" << p.occurrences << "\t" << p.successRate << "\n";
    }
    return true;
}

bool ErrorLearning::LoadKnowledge(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    m_patterns.clear();
    std::string line;
    while (std::getline(f, line)) {
        ErrorPattern p;
        size_t t1 = line.find('\t');
        if (t1 == std::string::npos) continue;
        p.pattern = line.substr(0, t1);
        size_t t2 = line.find('\t', t1 + 1);
        if (t2 == std::string::npos) continue;
        p.suggestedFix = line.substr(t1 + 1, t2 - t1 - 1);
        size_t t3 = line.find('\t', t2 + 1);
        if (t3 == std::string::npos) continue;
        try { p.occurrences = static_cast<uint32_t>(std::stoul(line.substr(t2 + 1, t3 - t2 - 1))); } catch (...) {}
        try { p.successRate = std::stof(line.substr(t3 + 1)); } catch (...) {}
        m_patterns.push_back(p);
    }
    return true;
}

size_t ErrorLearning::PatternCount() const { return m_patterns.size(); }
size_t ErrorLearning::ErrorCount() const { return m_errorHistory.size(); }

ErrorPattern* ErrorLearning::findOrCreatePattern(const std::string& errorCode) {
    for (auto& p : m_patterns) {
        if (p.pattern == errorCode) return &p;
    }
    ErrorPattern p;
    p.pattern = errorCode;
    m_patterns.push_back(p);
    return &m_patterns.back();
}

} // namespace AI
