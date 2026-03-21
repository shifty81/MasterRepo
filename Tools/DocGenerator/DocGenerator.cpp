#include "Tools/DocGenerator/DocGenerator.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;
namespace Tools {

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

DocGenerator::DocGenerator(DocGeneratorConfig cfg)
    : m_cfg(std::move(cfg))
{}

// ─────────────────────────────────────────────────────────────────────────────
// Scan
// ─────────────────────────────────────────────────────────────────────────────

int DocGenerator::Scan()
{
    m_fileDocs.clear();
    if (m_cfg.scanRoot.empty()) return 0;

    fs::path root(m_cfg.scanRoot);
    if (!fs::exists(root)) return 0;

    auto it_end = fs::end(fs::recursive_directory_iterator(root));
    auto it     = fs::begin(fs::recursive_directory_iterator(root));

    int count = 0;
    for (; it != it_end; ++it) {
        const auto& entry = *it;
        if (!entry.is_regular_file()) continue;

        auto ext = entry.path().extension().string();
        if (ext != ".h" && ext != ".hpp") continue;

        std::string rel = RelativePath(root, entry.path());
        if (MatchesExclude(rel, m_cfg.excludePatterns)) continue;

        FileDoc fd = ParseFile(entry.path());
        fd.filePath = rel;
        m_fileDocs.push_back(std::move(fd));
        ++count;
    }
    return count;
}

// ─────────────────────────────────────────────────────────────────────────────
// Generate
// ─────────────────────────────────────────────────────────────────────────────

int DocGenerator::Generate()
{
    if (m_cfg.outputDir.empty()) return 0;
    fs::create_directories(m_cfg.outputDir);

    int written = 0;

    for (const FileDoc& fd : m_fileDocs) {
        // e.g. outputDir/Core/Events/EventBus.md
        fs::path outPath = fs::path(m_cfg.outputDir) /
                           fs::path(fd.filePath).replace_extension(".md");
        fs::create_directories(outPath.parent_path());

        std::ofstream ofs(outPath);
        if (!ofs) continue;

        ofs << RenderFileDoc(fd);
        ++written;
    }

    if (m_cfg.generateIndex) {
        fs::path idxPath = fs::path(m_cfg.outputDir) / "index.md";
        std::ofstream ofs(idxPath);
        if (ofs) {
            ofs << RenderIndex();
            ++written;
        }
    }

    return written;
}

int DocGenerator::Run()
{
    Scan();
    return Generate();
}

// ─────────────────────────────────────────────────────────────────────────────
// Statistics
// ─────────────────────────────────────────────────────────────────────────────

int DocGenerator::ClassCount() const
{
    int n = 0;
    for (const auto& fd : m_fileDocs)
        n += static_cast<int>(fd.classes.size());
    return n;
}

int DocGenerator::FunctionCount() const
{
    int n = 0;
    for (const auto& fd : m_fileDocs) {
        n += static_cast<int>(fd.freeFunctions.size());
        for (const auto& cls : fd.classes)
            n += static_cast<int>(cls.publicMethods.size()) +
                 static_cast<int>(cls.staticMethods.size());
    }
    return n;
}

// ─────────────────────────────────────────────────────────────────────────────
// Parsing — FileDoc
// ─────────────────────────────────────────────────────────────────────────────

static std::string ReadFile(const fs::path& p)
{
    std::ifstream ifs(p);
    if (!ifs) return {};
    return {std::istreambuf_iterator<char>(ifs),
            std::istreambuf_iterator<char>()};
}

FileDoc DocGenerator::ParseFile(const fs::path& path)
{
    FileDoc fd;
    std::string src = ReadFile(path);
    if (src.empty()) return fd;

    // Extract file-level brief comment (first block comment)
    {
        static const std::regex reFileComment(R"(/\*\*?([\s\S]*?)\*/)");
        std::smatch m;
        if (std::regex_search(src, m, reFileComment))
            fd.briefComment = StripComment(m[1].str());
    }

    // Extract namespace
    {
        static const std::regex reNs(R"(namespace\s+(\w+)\s*\{)");
        std::smatch m;
        if (std::regex_search(src, m, reNs)) {
            // We'll put namespace on each class
        }
    }

    // Extract classes / structs
    {
        static const std::regex reCls(
            R"((?:/\*\*([\s\S]*?)\*/\s*)?(?:class|struct)\s+(\w+)"
            R"((?:\s*:\s*(?:public|protected|private)\s+(\w+))?\s*\{))");

        auto begin = std::sregex_iterator(src.begin(), src.end(), reCls);
        auto end   = std::sregex_iterator();

        for (auto it = begin; it != end; ++it) {
            const std::smatch& m = *it;
            ClassDoc cls;
            cls.briefComment = StripComment(m[1].str());
            cls.name         = m[2].str();
            cls.baseClass    = m[3].str();
            cls.headerPath   = fd.filePath;

            // Apply AI enrichment hook if provided and brief is empty
            if (cls.briefComment.empty() && m_cfg.aiEnrichHook)
                cls.briefComment = m_cfg.aiEnrichHook(cls);

            // Parse method lines within the class (simple heuristic)
            size_t openPos = m.position() + m.length() - 1; // position of '{'
            ParseClassBlock(src, openPos, cls);

            fd.classes.push_back(std::move(cls));
        }
    }

    // Extract free function declarations (not inside a class).
    // NOTE: This heuristic regex matches common single-line declarations ending
    // with ';'.  It intentionally does NOT match template functions, multi-line
    // declarations, or functions with trailing return types, as those require a
    // full parser.  The goal is a best-effort index, not 100% coverage.
    {
        static const std::regex reFunc(
            R"((?:/\*\*([\s\S]*?)\*/\s*)?(?:inline\s+)?(?:static\s+)?"
            R"([\w:<>*& ]+\s+(\w+)\s*\([^)]*\)\s*;))");

        auto begin = std::sregex_iterator(src.begin(), src.end(), reFunc);
        auto end   = std::sregex_iterator();
        for (auto it = begin; it != end; ++it) {
            const std::smatch& m = *it;
            FunctionDoc fn;
            fn.briefComment = StripComment(m[1].str());
            fn.name         = m[2].str();
            fn.signature    = m[0].str();
            fd.freeFunctions.push_back(std::move(fn));
        }
    }

    return fd;
}

void DocGenerator::ParseClassBlock(const std::string& src, size_t openBrace,
                                    ClassDoc& cls)
{
    // Find matching closing brace
    int depth = 1;
    size_t pos = openBrace + 1;
    while (pos < src.size() && depth > 0) {
        if (src[pos] == '{') ++depth;
        else if (src[pos] == '}') --depth;
        ++pos;
    }
    // pos now points just past the closing '}'
    std::string body = src.substr(openBrace + 1, pos - openBrace - 2);

    // Simple method extraction: lines ending in ';' that look like declarations
    static const std::regex reMethod(
        "(?:/\\*\\*([\\s\\S]*?)\\*/\\s*)?"
        "(?:virtual\\s+)?(?:static\\s+)?(?:inline\\s+)?"
        "[\\w:<>*&~ ]+\\s+(\\w+)\\s*\\([^)]*\\)(?:\\s*const)?(?:\\s*override)?\\s*;");

    bool inPublic = false;
    std::istringstream iss(body);
    std::string line;
    while (std::getline(iss, line)) {
        // Track access specifiers
        if (line.find("public:") != std::string::npos)   inPublic = true;
        if (line.find("private:") != std::string::npos)  inPublic = false;
        if (line.find("protected:") != std::string::npos)inPublic = false;

        if (!inPublic && !m_cfg.includePrivate) continue;

        std::smatch m;
        if (std::regex_search(line, m, reMethod)) {
            FunctionDoc fn;
            fn.briefComment = StripComment(m[1].str());
            fn.name         = m[2].str();
            fn.signature    = line;
            fn.isStatic     = line.find("static") != std::string::npos;
            fn.isVirtual    = line.find("virtual") != std::string::npos;
            fn.isConst      = line.find(") const") != std::string::npos;
            if (fn.isStatic)
                cls.staticMethods.push_back(std::move(fn));
            else
                cls.publicMethods.push_back(std::move(fn));
        }
    }
}

FunctionDoc DocGenerator::ParseFunctionDecl(const std::string& line,
                                              const std::string& comment)
{
    FunctionDoc fn;
    fn.signature    = line;
    fn.briefComment = StripComment(comment);

    static const std::regex reName(R"((\w+)\s*\()");
    std::smatch m;
    if (std::regex_search(line, m, reName))
        fn.name = m[1].str();

    return fn;
}

// ─────────────────────────────────────────────────────────────────────────────
// Markdown emitters
// ─────────────────────────────────────────────────────────────────────────────

std::string DocGenerator::RenderFuncDoc(const FunctionDoc& fn) const
{
    std::ostringstream oss;
    oss << "#### `" << fn.name << "`\n\n";
    if (!fn.briefComment.empty())
        oss << fn.briefComment << "\n\n";
    if (!fn.signature.empty())
        oss << "```cpp\n" << fn.signature << "\n```\n\n";
    return oss.str();
}

std::string DocGenerator::RenderClassDoc(const ClassDoc& cls) const
{
    std::ostringstream oss;
    oss << "### `" << cls.name << "`\n\n";
    if (!cls.baseClass.empty())
        oss << "*Inherits:* `" << cls.baseClass << "`\n\n";
    if (!cls.briefComment.empty())
        oss << cls.briefComment << "\n\n";
    if (!cls.headerPath.empty())
        oss << "*Declared in:* `" << cls.headerPath << "`\n\n";

    if (!cls.publicMethods.empty() || !cls.staticMethods.empty()) {
        oss << "**Public Methods**\n\n";
        for (const auto& fn : cls.publicMethods)
            oss << RenderFuncDoc(fn);
        for (const auto& fn : cls.staticMethods)
            oss << RenderFuncDoc(fn);
    }

    if (!cls.publicFields.empty()) {
        oss << "**Fields**\n\n";
        oss << "| Type | Name | Description |\n";
        oss << "|------|------|-------------|\n";
        for (const auto& f : cls.publicFields)
            oss << "| `" << f.type << "` | `" << f.name << "` | "
                << f.comment << " |\n";
        oss << "\n";
    }

    return oss.str();
}

std::string DocGenerator::RenderFileDoc(const FileDoc& fd) const
{
    std::ostringstream oss;

    // Title from filename
    std::string title = fs::path(fd.filePath).filename().replace_extension("").string();
    oss << "# " << title << "\n\n";
    if (!fd.briefComment.empty())
        oss << fd.briefComment << "\n\n";
    oss << "*Source:* `" << fd.filePath << "`\n\n---\n\n";

    // Classes
    if (!fd.classes.empty()) {
        oss << "## Classes\n\n";
        for (const auto& cls : fd.classes)
            oss << RenderClassDoc(cls);
    }

    // Free functions
    if (!fd.freeFunctions.empty()) {
        oss << "## Free Functions\n\n";
        for (const auto& fn : fd.freeFunctions)
            oss << RenderFuncDoc(fn);
    }

    return oss.str();
}

std::string DocGenerator::RenderIndex() const
{
    std::ostringstream oss;
    oss << "# " << m_cfg.projectName << " — API Reference\n\n";
    oss << "Auto-generated by Atlas DocGenerator.\n\n";
    oss << "| File | Classes | Functions |\n";
    oss << "|------|---------|----------|\n";

    for (const auto& fd : m_fileDocs) {
        int fnCount = static_cast<int>(fd.freeFunctions.size());
        for (const auto& cls : fd.classes)
            fnCount += static_cast<int>(cls.publicMethods.size()) +
                       static_cast<int>(cls.staticMethods.size());

        fs::path mdPath = fs::path(fd.filePath).replace_extension(".md");
        oss << "| [`" << fd.filePath << "`](" << mdPath.string() << ") | "
            << fd.classes.size() << " | " << fnCount << " |\n";
    }

    oss << "\n*Generated " << ClassCount() << " class(es) and "
        << FunctionCount() << " function(s).*\n";

    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// Utilities
// ─────────────────────────────────────────────────────────────────────────────

/* static */
bool DocGenerator::MatchesExclude(const std::string& rel,
                                   const std::vector<std::string>& patterns)
{
    for (const auto& pat : patterns) {
        // Simple substring / suffix matching
        if (rel.find(pat) != std::string::npos) return true;
    }
    return false;
}

/* static */
std::string DocGenerator::StripComment(const std::string& raw)
{
    if (raw.empty()) return {};
    // Remove leading * on each line, trim whitespace
    std::istringstream iss(raw);
    std::ostringstream oss;
    std::string line;
    bool first = true;
    while (std::getline(iss, line)) {
        // ltrim
        size_t start = line.find_first_not_of(" \t*");
        if (start == std::string::npos) continue;
        std::string trimmed = line.substr(start);
        // rtrim
        size_t end = trimmed.find_last_not_of(" \t\r");
        if (end != std::string::npos) trimmed = trimmed.substr(0, end + 1);
        if (trimmed.empty()) continue;
        if (!first) oss << ' ';
        oss << trimmed;
        first = false;
    }
    return oss.str();
}

/* static */
std::string DocGenerator::RelativePath(const fs::path& base,
                                        const fs::path& full)
{
    return fs::relative(full, base).string();
}

} // namespace Tools
