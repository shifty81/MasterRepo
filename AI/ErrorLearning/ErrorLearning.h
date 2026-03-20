#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace AI {

struct BuildError {
    std::string message;
    std::string file;
    uint32_t    line      = 0;
    std::string errorCode;
    uint64_t    timestamp = 0;
    bool        fixed     = false;
};

struct ErrorPattern {
    std::string pattern;
    std::string suggestedFix;
    uint32_t    occurrences  = 0;
    float       successRate  = 0.0f;
};

class ErrorLearning {
public:
    void        RecordError(const BuildError& error);
    void        RecordFix(const std::string& errorCode, const std::string& fix, bool success);
    std::string SuggestFix(const std::string& errorMessage) const;

    const std::vector<ErrorPattern>& GetPatterns() const;
    const std::vector<BuildError>&   GetErrorHistory() const;

    bool SaveKnowledge(const std::string& path) const;
    bool LoadKnowledge(const std::string& path);

    size_t PatternCount() const;
    size_t ErrorCount() const;

private:
    ErrorPattern* findOrCreatePattern(const std::string& errorCode);

    std::vector<BuildError>   m_errorHistory;
    std::vector<ErrorPattern> m_patterns;
};

} // namespace AI
