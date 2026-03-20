#include "WorkspaceState/WorkspaceState.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace Editor {

WorkspaceState& WorkspaceState::Instance() {
    static WorkspaceState instance;
    return instance;
}

void WorkspaceState::OpenFile(const std::string& path) {
    for (const auto& f : m_openFiles)
        if (f.path == path) return;
    m_openFiles.push_back({path, false, ""});
    if (m_activeFile.empty()) m_activeFile = path;
}

void WorkspaceState::CloseFile(const std::string& path) {
    m_openFiles.erase(
        std::remove_if(m_openFiles.begin(), m_openFiles.end(),
            [&](const FileEntry& f){ return f.path == path; }),
        m_openFiles.end());
    if (m_activeFile == path)
        m_activeFile = m_openFiles.empty() ? "" : m_openFiles.front().path;
}

void WorkspaceState::SetActiveFile(const std::string& path) { m_activeFile = path; }

void WorkspaceState::MarkDirty(const std::string& path) {
    for (auto& f : m_openFiles)
        if (f.path == path) { f.isDirty = true; return; }
}

void WorkspaceState::SetBuildState(BuildState state) { m_buildState = state; }

void WorkspaceState::AddError(const std::string& msg) { m_errors.push_back(msg); }
void WorkspaceState::AddWarning(const std::string& msg) { m_warnings.push_back(msg); }
void WorkspaceState::ClearErrors() { m_errors.clear(); }
void WorkspaceState::ClearWarnings() { m_warnings.clear(); }

const std::vector<FileEntry>& WorkspaceState::GetOpenFiles()  const { return m_openFiles; }
const std::string&            WorkspaceState::GetActiveFile() const { return m_activeFile; }
BuildState                    WorkspaceState::GetBuildState() const { return m_buildState; }
const std::vector<std::string>& WorkspaceState::GetErrors()   const { return m_errors; }
const std::vector<std::string>& WorkspaceState::GetWarnings() const { return m_warnings; }

bool WorkspaceState::Save(const std::string& path) const {
    std::ofstream out(path);
    if (!out.is_open()) return false;
    out << "active=" << m_activeFile << "\n";
    out << "build_state=" << static_cast<int>(m_buildState) << "\n";
    for (const auto& f : m_openFiles)
        out << "file=" << f.path << "|" << f.isDirty << "|" << f.lastModified << "\n";
    for (const auto& e : m_errors)   out << "error=" << e << "\n";
    for (const auto& w : m_warnings) out << "warning=" << w << "\n";
    return true;
}

bool WorkspaceState::Load(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) return false;
    Reset();
    std::string line;
    while (std::getline(in, line)) {
        if (line.substr(0, 7) == "active=") {
            m_activeFile = line.substr(7);
        } else if (line.substr(0, 12) == "build_state=") {
            m_buildState = static_cast<BuildState>(std::stoi(line.substr(12)));
        } else if (line.substr(0, 5) == "file=") {
            std::string val = line.substr(5);
            auto p1 = val.find('|');
            auto p2 = val.rfind('|');
            FileEntry entry;
            entry.path         = val.substr(0, p1);
            entry.isDirty      = (p1 != std::string::npos && p2 > p1) ? val.substr(p1 + 1, p2 - p1 - 1) == "1" : false;
            entry.lastModified = (p2 != std::string::npos) ? val.substr(p2 + 1) : "";
            m_openFiles.push_back(entry);
        } else if (line.substr(0, 6) == "error=") {
            m_errors.push_back(line.substr(6));
        } else if (line.substr(0, 8) == "warning=") {
            m_warnings.push_back(line.substr(8));
        }
    }
    return true;
}

void WorkspaceState::Reset() {
    m_openFiles.clear();
    m_activeFile.clear();
    m_buildState = BuildState::Clean;
    m_errors.clear();
    m_warnings.clear();
}

} // namespace Editor
