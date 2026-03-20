#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace AI {

struct ArchiveRecord {
    std::string path;
    std::string originalRepo;
    std::string language;
    std::string content;
    std::vector<std::string> tags;
    uint64_t indexedAt = 0;
};

struct ArchivePattern {
    std::string category;
    std::string codeSnippet;
    uint32_t    frequency = 0;
    float       quality   = 0.0f;
};

class ArchiveLearning {
public:
    void ScanArchiveDir(const std::string& dir);
    void IndexRecord(ArchiveRecord rec);
    void ExtractPatterns();

    std::vector<ArchiveRecord>  Search(const std::string& query) const;
    std::vector<ArchiveRecord>  ByTag(const std::string& tag) const;
    std::vector<ArchivePattern> GetPatterns() const;
    std::vector<ArchivePattern> GetPatternsByCategory(const std::string& cat) const;

    std::string GenerateTrainingSample(const ArchiveRecord& rec) const;

    bool   SaveIndex(const std::string& path) const;
    bool   LoadIndex(const std::string& path);
    size_t RecordCount() const;
    size_t PatternCount() const;
    void   Clear();

private:
    std::string              DetectLanguage(const std::string& path) const;
    std::vector<std::string> AutoTag(const std::string& content, const std::string& lang) const;

    std::vector<ArchiveRecord>  m_records;
    std::vector<ArchivePattern> m_patterns;
};

} // namespace AI
