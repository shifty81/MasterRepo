#pragma once
#include <string>
#include <vector>

namespace Tools {

struct GitFileStatus {
    std::string path;
    std::string indexStatus;
    std::string workTreeStatus;
};

struct GitCommit {
    std::string hash;
    std::string shortHash;
    std::string author;
    std::string date;
    std::string message;
};

struct GitBranch {
    std::string name;
    bool        isCurrent = false;
    std::string trackingBranch;
};

class GitPanel {
public:
    explicit GitPanel(const std::string& repoRoot);

    std::vector<GitFileStatus> GetStatus() const;
    bool   HasUncommittedChanges() const;
    std::string GetCurrentBranch() const;

    std::string GetDiff(const std::string& file = "") const;
    std::string GetStagedDiff() const;

    bool StageFile(const std::string& path);
    bool StageAll();
    bool UnstageFile(const std::string& path);

    bool Commit(const std::string& message);
    bool AmendCommit(const std::string& message);

    std::vector<GitBranch> GetBranches() const;
    bool CreateBranch(const std::string& name);
    bool CheckoutBranch(const std::string& name);
    bool DeleteBranch(const std::string& name, bool force = false);

    std::vector<GitCommit> GetLog(int maxCount = 50) const;

    bool Fetch();
    bool Pull();
    bool Push(const std::string& remote = "origin", const std::string& branch = "");

    std::string RunGit(const std::vector<std::string>& args) const;

    std::string Name() const  { return "Git"; }
    bool IsVisible() const    { return m_visible; }
    void SetVisible(bool v)   { m_visible = v; }

private:
    std::string   m_repoRoot;
    bool          m_visible = true;

    std::string   buildCmd(const std::vector<std::string>& args) const;
    GitFileStatus parseStatusLine(const std::string& line) const;
    GitCommit     parseLogLine(const std::string& line) const;
};

} // namespace Tools
