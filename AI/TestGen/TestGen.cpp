#include "AI/TestGen/TestGen.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <filesystem>

namespace AI {

TestGen::TestGen(ModelManager& mm) : m_mm(mm) {}

// ── configuration ─────────────────────────────────────────────────────────────

void   TestGen::SetMaxTests(size_t max)     { m_maxTests  = max; }
size_t TestGen::MaxTests()           const  { return m_maxTests; }
void   TestGen::SetFramework(TestFramework fw) { m_framework = fw; }
size_t TestGen::TotalGenerated()     const  { return m_totalGenerated; }
void   TestGen::ResetStats()                { m_totalGenerated = 0; }

// ── preamble ──────────────────────────────────────────────────────────────────

std::string TestGen::frameworkPreamble(TestFramework fw) const {
    switch (fw) {
        case TestFramework::CppCatch2:
            return "#define CATCH_CONFIG_MAIN\n#include <catch2/catch.hpp>\n";
        case TestFramework::CppGoogleTest:
            return "#include <gtest/gtest.h>\n";
        case TestFramework::CppCustom:
        default:
            return "#include <cassert>\n#include <iostream>\n#include <stdexcept>\n\n"
                   "#define TEST_ASSERT(cond) do { if (!(cond)) { "
                   "std::cerr << \"FAIL: \" #cond << \" (\" << __FILE__ << \":\" << __LINE__ << \")\\n\"; } "
                   "else { std::cout << \"PASS: \" #cond << \"\\n\"; } } while(0)\n";
    }
}

// ── prompt builder ────────────────────────────────────────────────────────────

std::string TestGen::buildPrompt(const std::string& code, const std::string& target) const {
    std::ostringstream ss;
    ss << "You are a C++20 unit test generator.\n"
       << "Write a concise C++ test function for the following function/class: `" << target << "`.\n"
       << "Requirements:\n"
       << "- Use plain C++ with no external test framework (just assert or TEST_ASSERT macro).\n"
       << "- The test function should be named: Test_" << target << "\n"
       << "- Return void, no parameters.\n"
       << "- Include edge cases and typical cases.\n"
       << "- Output ONLY the test function body inside a ```cpp ... ``` code block.\n\n"
       << "Source code:\n```cpp\n" << code << "\n```\n";
    return ss.str();
}

// ── response parser ───────────────────────────────────────────────────────────

TestCase TestGen::parseTestFromResponse(const std::string& response,
                                         const std::string& target) const {
    TestCase tc;
    tc.targetFunction = target;
    tc.name           = "Test_" + target;
    tc.description    = "AI-generated test for " + target;

    // Try to extract ```cpp ... ``` block first.
    auto codeBlock = [&](const std::string& fence) -> std::string {
        auto start = response.find(fence);
        if (start == std::string::npos) return {};
        start += fence.size();
        auto end = response.find("```", start);
        if (end == std::string::npos) return {};
        return response.substr(start, end - start);
    };

    std::string extracted = codeBlock("```cpp\n");
    if (extracted.empty()) extracted = codeBlock("```c++\n");
    if (extracted.empty()) extracted = codeBlock("```\n");

    if (!extracted.empty()) {
        tc.code = extracted;
        return tc;
    }

    // Fallback: look for a void Test... pattern.
    std::regex funcPat(R"(void\s+Test\w+\s*\([^)]*\)\s*\{)");
    std::smatch m;
    if (std::regex_search(response, m, funcPat)) {
        auto start = m.position();
        int depth = 0;
        size_t i = static_cast<size_t>(start);
        while (i < response.size()) {
            if (response[i] == '{') ++depth;
            else if (response[i] == '}') { if (--depth == 0) { ++i; break; } }
            ++i;
        }
        tc.code = response.substr(static_cast<size_t>(start), i - static_cast<size_t>(start));
        return tc;
    }

    // Last resort: store raw response.
    tc.code = "// AI response:\n// " + response.substr(0, std::min(response.size(), size_t(200)));
    return tc;
}

// ── generate for symbol ───────────────────────────────────────────────────────

std::vector<TestCase> TestGen::GenerateForSymbol(const std::string& symbol,
                                                   const std::string& sourceCode) {
    std::vector<TestCase> cases;
    std::string prompt = buildPrompt(sourceCode, symbol);
    auto resp = m_mm.Complete(prompt);

    TestCase tc = parseTestFromResponse(resp.text, symbol);
    tc.targetFunction = symbol;
    cases.push_back(tc);
    ++m_totalGenerated;
    return cases;
}

// ── generate for file ─────────────────────────────────────────────────────────

TestSuite TestGen::GenerateForFile(const std::string& filePath, TestFramework framework) {
    TestSuite suite;
    suite.targetFile = filePath;
    suite.framework  = framework;
    suite.name       = "Tests_" + std::filesystem::path(filePath).stem().string();
    suite.preamble   = frameworkPreamble(framework);

    std::ifstream f(filePath);
    if (!f) return suite;
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string src = ss.str();

    // Extract function signatures: return_type Name(
    std::regex sigPat(R"((?:void|int|bool|float|double|auto|std::\w+)\s+(\w+)\s*\([^)]*\))");
    std::sregex_iterator it(src.begin(), src.end(), sigPat), end;

    size_t count = 0;
    for (; it != end && count < m_maxTests; ++it, ++count) {
        std::string sym = (*it)[1].str();
        // Skip common non-test targets.
        if (sym == "main" || sym == "operator") continue;
        auto cases = GenerateForSymbol(sym, src);
        for (auto& c : cases) {
            c.targetFile = filePath;
            suite.tests.push_back(c);
        }
    }
    return suite;
}

// ── generate from errors ──────────────────────────────────────────────────────

std::vector<TestCase> TestGen::GenerateFromErrors(const std::vector<std::string>& errorLines) {
    std::vector<TestCase> cases;
    for (auto& line : errorLines) {
        if (cases.size() >= m_maxTests) break;
        std::string prompt =
            "You are a C++20 regression test generator.\n"
            "The following error was observed:\n  " + line + "\n"
            "Write a minimal C++ test function (void, no params) that reproduces or guards "
            "against this error. Output ONLY a ```cpp ... ``` code block.\n";
        auto resp = m_mm.Complete(prompt);
        TestCase tc = parseTestFromResponse(resp.text, "Regression");
        tc.description = "Regression for: " + line.substr(0, 80);
        tc.tags.push_back("regression");
        cases.push_back(tc);
        ++m_totalGenerated;
    }
    return cases;
}

// ── write test suite ──────────────────────────────────────────────────────────

bool TestGen::WriteTestSuite(const TestSuite& suite, const std::string& outputPath) const {
    std::ofstream f(outputPath);
    if (!f) return false;

    f << suite.preamble << "\n";
    f << "// Auto-generated test suite for: " << suite.targetFile << "\n\n";

    for (auto& tc : suite.tests) {
        f << "// Test: " << tc.name << "\n";
        if (!tc.description.empty())
            f << "// Description: " << tc.description << "\n";
        f << tc.code << "\n\n";
    }

    // Simple main() for CppCustom framework.
    if (suite.framework == TestFramework::CppCustom) {
        f << "int main() {\n";
        for (auto& tc : suite.tests) {
            // Call test functions if they follow the void Test_X() pattern.
            if (!tc.code.empty() && tc.code.find("void Test_") != std::string::npos)
                f << "    " << tc.name << "();\n";
        }
        f << "    std::cout << \"All tests completed.\\n\";\n";
        f << "    return 0;\n}\n";
    }

    return f.good();
}

} // namespace AI
