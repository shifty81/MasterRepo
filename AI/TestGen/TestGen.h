#pragma once
#include "AI/ModelManager/ModelManager.h"
#include <string>
#include <vector>
#include <functional>

namespace AI {

enum class TestFramework { CppCatch2, CppGoogleTest, CppCustom };

struct TestCase {
    std::string name;
    std::string description;
    std::string code;
    std::string targetFunction;
    std::string targetFile;
    bool        isPassingTest = true;
    std::vector<std::string> tags;
};

struct TestSuite {
    std::string           name;
    std::string           targetFile;
    TestFramework         framework = TestFramework::CppCustom;
    std::vector<TestCase> tests;
    std::string           preamble;
};

class TestGen {
public:
    explicit TestGen(ModelManager& mm);

    TestSuite GenerateForFile(const std::string& filePath,
                              TestFramework framework = TestFramework::CppCustom);

    std::vector<TestCase> GenerateForSymbol(const std::string& symbol,
                                             const std::string& sourceCode);

    std::vector<TestCase> GenerateFromErrors(const std::vector<std::string>& errorLines);

    bool WriteTestSuite(const TestSuite& suite, const std::string& outputPath) const;

    void   SetMaxTests(size_t max);
    size_t MaxTests() const;
    void   SetFramework(TestFramework fw);

    size_t TotalGenerated() const;
    void   ResetStats();

private:
    std::string buildPrompt(const std::string& code, const std::string& target) const;
    TestCase    parseTestFromResponse(const std::string& response,
                                      const std::string& target) const;
    std::string frameworkPreamble(TestFramework fw) const;

    ModelManager& m_mm;
    TestFramework m_framework     = TestFramework::CppCustom;
    size_t        m_maxTests      = 10;
    size_t        m_totalGenerated = 0;
};

} // namespace AI
