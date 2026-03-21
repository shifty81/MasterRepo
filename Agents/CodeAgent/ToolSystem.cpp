#include "Agents/CodeAgent/ToolSystem.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

#ifdef _WIN32
#  define popen  _popen
#  define pclose _pclose
#endif

namespace Agents {

// Reject strings containing shell metacharacters to prevent command injection.
// Blocks a broad set of characters that could be used in shell injection attacks.
static bool IsSafePath(const std::string& s) {
    for (char c : s) {
        if (c == ';' || c == '&' || c == '|' || c == '`' || c == '$' ||
            c == '(' || c == ')' || c == '<' || c == '>' || c == '\n' ||
            c == '\r' || c == '*' || c == '?' || c == '[' || c == ']' ||
            c == '{' || c == '}' || c == '!' || c == '~' || c == '\'' ||
            c == '"' || static_cast<unsigned char>(c) < 0x20)
            return false;
    }
    return !s.empty();
}

// ReadFileTool
std::string ReadFileTool::Name() const { return "read_file"; }
std::string ReadFileTool::Description() const { return "Read the contents of a file"; }
std::vector<ToolParam> ReadFileTool::Params() const {
    return {{"path", "string", true, "Absolute or relative file path"}};
}
ToolResult ReadFileTool::Execute(const std::map<std::string,std::string>& params) {
    auto it = params.find("path");
    if (it == params.end()) return {false, "", "Missing required param: path"};
    std::ifstream f(it->second);
    if (!f) return {false, "", "Cannot open file: " + it->second};
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return {true, content, ""};
}

// WriteFileTool
std::string WriteFileTool::Name() const { return "write_file"; }
std::string WriteFileTool::Description() const { return "Write content to a file"; }
std::vector<ToolParam> WriteFileTool::Params() const {
    return {{"path", "string", true, "File path"}, {"content", "string", true, "Content to write"}};
}
ToolResult WriteFileTool::Execute(const std::map<std::string,std::string>& params) {
    auto pit = params.find("path");
    auto cit = params.find("content");
    if (pit == params.end()) return {false, "", "Missing param: path"};
    if (cit == params.end()) return {false, "", "Missing param: content"};
    std::ofstream f(pit->second);
    if (!f) return {false, "", "Cannot write file: " + pit->second};
    f << cit->second;
    return {true, "Written " + std::to_string(cit->second.size()) + " bytes", ""};
}

// ListDirTool
std::string ListDirTool::Name() const { return "list_dir"; }
std::string ListDirTool::Description() const { return "List files in a directory"; }
std::vector<ToolParam> ListDirTool::Params() const {
    return {{"path", "string", true, "Directory path"}};
}
ToolResult ListDirTool::Execute(const std::map<std::string,std::string>& params) {
    auto it = params.find("path");
    if (it == params.end()) return {false, "", "Missing param: path"};
    namespace fs = std::filesystem;
    if (!fs::exists(it->second)) return {false, "", "Path does not exist: " + it->second};
    std::string out;
    for (auto& entry : fs::directory_iterator(it->second)) {
        out += entry.path().filename().string() + "\n";
    }
    return {true, out, ""};
}

// SearchCodeTool
std::string SearchCodeTool::Name() const { return "search_code"; }
std::string SearchCodeTool::Description() const { return "Search for a pattern in code files"; }
std::vector<ToolParam> SearchCodeTool::Params() const {
    return {{"path", "string", true, "Directory to search"}, {"pattern", "string", true, "Text pattern"}};
}
ToolResult SearchCodeTool::Execute(const std::map<std::string,std::string>& params) {
    auto pit = params.find("path");
    auto pat = params.find("pattern");
    if (pit == params.end()) return {false, "", "Missing param: path"};
    if (pat == params.end()) return {false, "", "Missing param: pattern"};
    namespace fs = std::filesystem;
    if (!fs::exists(pit->second)) return {false, "", "Path not found"};
    std::string out;
    for (auto& entry : fs::recursive_directory_iterator(pit->second)) {
        if (!entry.is_regular_file()) continue;
        std::ifstream f(entry.path());
        if (!f) continue;
        std::string line;
        uint32_t lineNum = 0;
        while (std::getline(f, line)) {
            ++lineNum;
            if (line.find(pat->second) != std::string::npos) {
                out += entry.path().string() + ":" + std::to_string(lineNum) + ": " + line + "\n";
            }
        }
    }
    return {true, out, ""};
}

// CompileTool
std::string CompileTool::Name() const { return "compile"; }
std::string CompileTool::Description() const { return "Compile a C++ source file with g++"; }
std::vector<ToolParam> CompileTool::Params() const {
    return {{"file", "string", true, "Source file to compile"}, {"flags", "string", false, "Extra compiler flags"}};
}
ToolResult CompileTool::Execute(const std::map<std::string,std::string>& params) {
    auto fit = params.find("file");
    if (fit == params.end()) return {false, "", "Missing param: file"};
    if (!IsSafePath(fit->second)) return {false, "", "Unsafe characters in file path"};
    std::string cmd = "g++ -std=c++20 -c " + fit->second;
    auto flit = params.find("flags");
    if (flit != params.end()) {
        if (!IsSafePath(flit->second)) return {false, "", "Unsafe characters in flags"};
        cmd += " " + flit->second;
    }
    cmd += " 2>&1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return {false, "", "Failed to run compiler"};
    std::string output;
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe)) output += buf;
    int ret = pclose(pipe);
    return {ret == 0, output, ret != 0 ? "Compilation failed" : ""};
}

// RunCMakeTool
std::string RunCMakeTool::Name() const { return "run_cmake"; }
std::string RunCMakeTool::Description() const { return "Run CMake configure or build"; }
std::vector<ToolParam> RunCMakeTool::Params() const {
    return {{"source_dir", "string", true, "CMake source directory"},
            {"build_dir",  "string", true, "CMake build directory"},
            {"build",      "string", false, "If 'true', run cmake --build"}};
}
ToolResult RunCMakeTool::Execute(const std::map<std::string,std::string>& params) {
    auto sit = params.find("source_dir");
    auto bit = params.find("build_dir");
    if (sit == params.end()) return {false, "", "Missing param: source_dir"};
    if (bit == params.end()) return {false, "", "Missing param: build_dir"};
    if (!IsSafePath(sit->second)) return {false, "", "Unsafe characters in source_dir"};
    if (!IsSafePath(bit->second)) return {false, "", "Unsafe characters in build_dir"};
    std::string cmd;
    auto doBuild = params.find("build");
    if (doBuild != params.end() && doBuild->second == "true") {
        cmd = "cmake --build " + bit->second + " 2>&1";
    } else {
        cmd = "cmake -S " + sit->second + " -B " + bit->second + " 2>&1";
    }
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return {false, "", "Failed to run cmake"};
    std::string output;
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe)) output += buf;
    int ret = pclose(pipe);
    return {ret == 0, output, ret != 0 ? "CMake failed" : ""};
}

// ToolRegistry
ToolRegistry::ToolRegistry() {
    Register(std::make_unique<ReadFileTool>());
    Register(std::make_unique<WriteFileTool>());
    Register(std::make_unique<ListDirTool>());
    Register(std::make_unique<SearchCodeTool>());
    Register(std::make_unique<CompileTool>());
    Register(std::make_unique<RunCMakeTool>());
}

void ToolRegistry::Register(std::unique_ptr<ITool> tool) { m_tools.push_back(std::move(tool)); }

ITool* ToolRegistry::Get(const std::string& name) {
    for (auto& t : m_tools) if (t->Name() == name) return t.get();
    return nullptr;
}

ToolResult ToolRegistry::Execute(const std::string& name, const std::map<std::string,std::string>& params) {
    auto* tool = Get(name);
    if (!tool) return {false, "", "Tool not found: " + name};
    return tool->Execute(params);
}

std::vector<std::string> ToolRegistry::List() const {
    std::vector<std::string> names;
    for (const auto& t : m_tools) names.push_back(t->Name());
    return names;
}

size_t ToolRegistry::Count() const { return m_tools.size(); }

} // namespace Agents
