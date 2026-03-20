#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace Agents {

struct ToolParam {
    std::string name;
    std::string type;
    bool        required    = true;
    std::string description;
};

struct ToolResult {
    bool        success     = false;
    std::string output;
    std::string errorMessage;
};

class ITool {
public:
    virtual ~ITool() = default;
    virtual std::string            Name() const = 0;
    virtual std::string            Description() const = 0;
    virtual std::vector<ToolParam> Params() const = 0;
    virtual ToolResult             Execute(const std::map<std::string,std::string>& params) = 0;
};

class ReadFileTool : public ITool {
public:
    std::string Name() const override;
    std::string Description() const override;
    std::vector<ToolParam> Params() const override;
    ToolResult Execute(const std::map<std::string,std::string>& params) override;
};

class WriteFileTool : public ITool {
public:
    std::string Name() const override;
    std::string Description() const override;
    std::vector<ToolParam> Params() const override;
    ToolResult Execute(const std::map<std::string,std::string>& params) override;
};

class ListDirTool : public ITool {
public:
    std::string Name() const override;
    std::string Description() const override;
    std::vector<ToolParam> Params() const override;
    ToolResult Execute(const std::map<std::string,std::string>& params) override;
};

class SearchCodeTool : public ITool {
public:
    std::string Name() const override;
    std::string Description() const override;
    std::vector<ToolParam> Params() const override;
    ToolResult Execute(const std::map<std::string,std::string>& params) override;
};

class CompileTool : public ITool {
public:
    std::string Name() const override;
    std::string Description() const override;
    std::vector<ToolParam> Params() const override;
    ToolResult Execute(const std::map<std::string,std::string>& params) override;
};

class RunCMakeTool : public ITool {
public:
    std::string Name() const override;
    std::string Description() const override;
    std::vector<ToolParam> Params() const override;
    ToolResult Execute(const std::map<std::string,std::string>& params) override;
};

class ToolRegistry {
public:
    ToolRegistry();

    void         Register(std::unique_ptr<ITool> tool);
    ITool*       Get(const std::string& name);
    ToolResult   Execute(const std::string& name, const std::map<std::string,std::string>& params);
    std::vector<std::string> List() const;
    size_t       Count() const;

private:
    std::vector<std::unique_ptr<ITool>> m_tools;
};

} // namespace Agents
