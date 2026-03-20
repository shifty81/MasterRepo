#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace Editor {

enum class BuildState : uint8_t {
    Clean,
    Dirty,
    Building,
    Error,
};

struct FileEntry {
    std::string path;
    bool        isDirty      = false;
    std::string lastModified;
};

class WorkspaceState {
public:
    static WorkspaceState& Instance();

    void OpenFile(const std::string& path);
    void CloseFile(const std::string& path);
    void SetActiveFile(const std::string& path);
    void MarkDirty(const std::string& path);

    void SetBuildState(BuildState state);
    void AddError(const std::string& msg);
    void AddWarning(const std::string& msg);
    void ClearErrors();
    void ClearWarnings();

    const std::vector<FileEntry>& GetOpenFiles() const;
    const std::string& GetActiveFile() const;
    BuildState GetBuildState() const;
    const std::vector<std::string>& GetErrors() const;
    const std::vector<std::string>& GetWarnings() const;

    bool Save(const std::string& path) const;
    bool Load(const std::string& path);

    void Reset();

private:
    WorkspaceState() = default;

    std::vector<FileEntry>   m_openFiles;
    std::string              m_activeFile;
    BuildState               m_buildState = BuildState::Clean;
    std::vector<std::string> m_errors;
    std::vector<std::string> m_warnings;
};

} // namespace Editor
