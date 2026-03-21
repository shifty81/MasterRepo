#include "Tools/DependencyAnalyzer/DependencyAnalyzer.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <stack>
#include <queue>

namespace Tools {

// ── IncludeCycle ──────────────────────────────────────────────────────────
std::string IncludeCycle::Description() const {
    std::string s;
    for (size_t i = 0; i < chain.size(); ++i) {
        s += chain[i];
        if (i + 1 < chain.size()) s += " → ";
    }
    s += " (cycle)";
    return s;
}

// ── TopFanout/Fanin ───────────────────────────────────────────────────────
static std::string topByMetric(
    const std::unordered_map<std::string, IncludeNode>& nodes,
    size_t n, bool byFanout)
{
    std::vector<std::pair<uint32_t,std::string>> v;
    for (const auto& [p, node] : nodes)
        v.push_back({byFanout ? node.fanout : node.fanin, p});
    std::sort(v.rbegin(), v.rend());
    std::ostringstream ss;
    for (size_t i = 0; i < std::min(n, v.size()); ++i)
        ss << v[i].first << "\t" << v[i].second << "\n";
    return ss.str();
}

std::string DependencyReport::TopFanout(size_t n) const { return topByMetric(nodes, n, true); }
std::string DependencyReport::TopFanin(size_t n)  const { return topByMetric(nodes, n, false); }

// ── Impl ─────────────────────────────────────────────────────────────────
struct DependencyAnalyzer::Impl {
    std::vector<std::string> roots;
    std::vector<std::string> includePaths;
    std::vector<std::string> extensions;
    bool followSystem{false};
    std::vector<ProgressCb>  cbs;

    std::string resolve(const std::string& inc) const {
        namespace fs = std::filesystem;
        if (fs::exists(inc)) return fs::canonical(inc).string();
        for (const auto& p : includePaths) {
            auto full = fs::path(p) / inc;
            if (fs::exists(full)) return fs::canonical(full).string();
        }
        return inc; // unresolved
    }

    std::vector<std::string> parseIncludes(const std::string& path, bool& ok) const {
        ok = false;
        std::ifstream f(path);
        if (!f.is_open()) return {};
        ok = true;
        static const std::regex re(R"(#\s*include\s*[<"]([^>"]+)[>"])");
        std::vector<std::string> result;
        std::string line;
        bool isSystem = false;
        while (std::getline(f, line)) {
            std::smatch m;
            if (std::regex_search(line, m, re)) {
                bool sys = line.find('<') != std::string::npos;
                if (sys && !followSystem) continue;
                result.push_back(m[1].str());
                (void)isSystem;
            }
        }
        return result;
    }
};

DependencyAnalyzer::DependencyAnalyzer() : m_impl(new Impl()) {
    for (const char* e : {".h",".hpp",".hxx",".c",".cpp",".cxx",".cc"})
        m_impl->extensions.push_back(e);
}
DependencyAnalyzer::~DependencyAnalyzer() { delete m_impl; }

void DependencyAnalyzer::AddSearchRoot(const std::string& d)    { m_impl->roots.push_back(d); }
void DependencyAnalyzer::AddIncludePath(const std::string& d)   { m_impl->includePaths.push_back(d); }
void DependencyAnalyzer::AddExtension(const std::string& e)     { m_impl->extensions.push_back(e); }
void DependencyAnalyzer::SetFollowSystemIncludes(bool f)        { m_impl->followSystem = f; }
void DependencyAnalyzer::OnProgress(ProgressCb cb)              { m_impl->cbs.push_back(std::move(cb)); }

DependencyReport DependencyAnalyzer::Analyse() {
    namespace fs = std::filesystem;
    DependencyReport report;

    // Collect all files
    std::vector<std::string> allFiles;
    for (const auto& root : m_impl->roots) {
        if (!fs::exists(root)) continue;
        for (const auto& entry : fs::recursive_directory_iterator(root,
             fs::directory_options::skip_permission_denied))
        {
            if (!entry.is_regular_file()) continue;
            std::string ext = entry.path().extension().string();
            for (const auto& e : m_impl->extensions)
                if (e == ext) { allFiles.push_back(entry.path().string()); break; }
        }
    }

    size_t total = allFiles.size();
    size_t scanned = 0;
    for (const auto& path : allFiles) {
        bool ok;
        auto incs = m_impl->parseIncludes(path, ok);
        if (!ok) { ++scanned; continue; }

        IncludeNode node;
        node.filePath        = path;
        node.directIncludes  = incs;
        for (const auto& inc : incs) node.resolvedIncludes.push_back(m_impl->resolve(inc));
        node.fanout = static_cast<uint32_t>(incs.size());
        report.nodes[path] = std::move(node);
        ++scanned;
        for (auto& cb : m_impl->cbs) cb(scanned, total);
    }

    // Compute fanin
    for (auto& [file, node] : report.nodes) {
        for (const auto& inc : node.resolvedIncludes) {
            if (report.nodes.count(inc)) report.nodes[inc].fanin++;
        }
    }

    // Roots (fanin == 0) and leaves (fanout == 0)
    for (const auto& [file, node] : report.nodes) {
        if (node.fanin  == 0) report.roots.push_back(file);
        if (node.fanout == 0) report.leaves.push_back(file);
    }

    // Cycle detection via DFS coloring
    enum Color : uint8_t { White, Gray, Black };
    std::unordered_map<std::string, Color> color;
    for (const auto& [f, _] : report.nodes) color[f] = White;

    std::function<void(const std::string&, std::vector<std::string>&)> dfs;
    dfs = [&](const std::string& file, std::vector<std::string>& path2) {
        auto it = report.nodes.find(file);
        if (it == report.nodes.end()) return;
        color[file] = Gray;
        path2.push_back(file);
        for (const auto& dep : it->second.resolvedIncludes) {
            if (!report.nodes.count(dep)) continue;
            if (color[dep] == Gray) {
                // Found cycle: extract chain
                IncludeCycle cycle;
                auto pos = std::find(path2.begin(), path2.end(), dep);
                cycle.chain.assign(pos, path2.end());
                cycle.chain.push_back(dep);
                report.cycles.push_back(cycle);
            } else if (color[dep] == White) {
                dfs(dep, path2);
            }
        }
        path2.pop_back();
        color[file] = Black;
    };

    for (const auto& [f, _] : report.nodes)
        if (color[f] == White) {
            std::vector<std::string> p;
            dfs(f, p);
        }

    return report;
}

DependencyReport DependencyAnalyzer::AnalyseFile(const std::string& path) {
    DependencyReport report;
    bool ok;
    auto incs = m_impl->parseIncludes(path, ok);
    if (!ok) return report;
    IncludeNode node;
    node.filePath       = path;
    node.directIncludes = incs;
    for (const auto& i : incs) node.resolvedIncludes.push_back(m_impl->resolve(i));
    node.fanout = static_cast<uint32_t>(incs.size());
    report.nodes[path] = std::move(node);
    return report;
}

std::vector<std::string> DependencyAnalyzer::FindDependents(
    const std::string& target, const DependencyReport& report) const
{
    std::vector<std::string> out;
    for (const auto& [file, node] : report.nodes) {
        for (const auto& dep : node.resolvedIncludes) {
            if (dep == target) { out.push_back(file); break; }
        }
    }
    return out;
}

std::vector<std::string> DependencyAnalyzer::FindDependencies(
    const std::string& source, const DependencyReport& report) const
{
    std::vector<std::string> out;
    auto it = report.nodes.find(source);
    if (it == report.nodes.end()) return out;
    // BFS
    std::queue<std::string> q;
    std::unordered_map<std::string,bool> visited;
    for (const auto& dep : it->second.resolvedIncludes) { q.push(dep); visited[dep]=true; }
    while (!q.empty()) {
        auto f = q.front(); q.pop();
        out.push_back(f);
        auto jt = report.nodes.find(f);
        if (jt == report.nodes.end()) continue;
        for (const auto& dep : jt->second.resolvedIncludes)
            if (!visited[dep]) { visited[dep]=true; q.push(dep); }
    }
    return out;
}

std::string DependencyAnalyzer::ExportText(const DependencyReport& report) const {
    std::ostringstream ss;
    ss << "Files: " << report.FileCount() << "\n";
    ss << "Cycles: " << report.CycleCount() << "\n\n";
    ss << "=== Top Fanout ===\n" << report.TopFanout();
    ss << "\n=== Top Fanin ===\n"  << report.TopFanin();
    if (!report.cycles.empty()) {
        ss << "\n=== Cycles ===\n";
        for (const auto& c : report.cycles) ss << c.Description() << "\n";
    }
    return ss.str();
}

std::string DependencyAnalyzer::ExportDOT(const DependencyReport& report,
                                           const std::string& graphName) const
{
    std::ostringstream ss;
    ss << "digraph " << graphName << " {\n  rankdir=LR;\n";
    for (const auto& [file, node] : report.nodes) {
        std::string shortFile = std::filesystem::path(file).filename().string();
        for (const auto& dep : node.resolvedIncludes) {
            std::string shortDep = std::filesystem::path(dep).filename().string();
            ss << "  \"" << shortFile << "\" -> \"" << shortDep << "\";\n";
        }
    }
    ss << "}\n";
    return ss.str();
}

} // namespace Tools
