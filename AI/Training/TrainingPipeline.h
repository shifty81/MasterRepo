#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace AI {

enum class SampleSource { Archive, AssetIndex, ErrorFix, CodeGen, UserCorrection };

struct TrainingSample {
    std::string  prompt;
    std::string  completion;
    SampleSource source    = SampleSource::Archive;
    float        quality   = 0.0f;
    std::vector<std::string> tags;
    uint64_t     timestamp = 0;
};

struct PipelineStats {
    size_t totalCollected = 0;
    size_t afterFilter    = 0;
    size_t totalExported  = 0;
    float  avgQuality     = 0.0f;
};

class TrainingPipeline {
public:
    void AddSample(TrainingSample s);
    void CollectFromArchiveLearning(const std::string& indexPath);
    void CollectFromErrorFixes(const std::string& knowledgePath);

    void FilterByQuality(float minQuality);
    void FilterBySource(SampleSource src);
    void DeduplicateByPrompt();

    bool ExportJSONL(const std::string& outputPath) const;
    bool ExportAlpaca(const std::string& outputPath) const;

    PipelineStats GetStats() const;
    size_t SampleCount() const;
    void   Clear();
    const std::vector<TrainingSample>& Samples() const;

private:
    std::vector<TrainingSample> m_samples;
    mutable size_t              m_totalExported = 0;
};

} // namespace AI
