#pragma once
#include <string>
#include <vector>

namespace PCG {
namespace Advanced {

class PCGAdvanced {
public:
    // Generate rules using AI
    std::vector<std::string> GenerateRules(const std::string& prompt);
    // Validate generated content
    bool ValidateContent(const std::string& content);
    // Provide live preview data
    std::string GetLivePreview(const std::string& config);
};

} // namespace Advanced
} // namespace PCG
