#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace Editor {

enum class ErrorSeverity { Error, Warning, Note, Info };

struct ParsedError {
    ErrorSeverity severity = ErrorSeverity::Error;
    std::string   file;
    uint32_t      line   = 0;
    uint32_t      column = 0;
    std::string   message;
    std::string   rawLine;
};

class ErrorPanel {
public:
    std::string Name() const   { return "Errors"; }
    bool IsVisible() const     { return m_visible; }
    void SetVisible(bool v)    { m_visible = v; }

    void ParseBuildOutput(const std::string& compilerOutput);
    void AddError(ParsedError e);
    void Clear();

    const std::vector<ParsedError>& GetErrors()   const;
    const std::vector<ParsedError>& GetWarnings() const;

    using NavigateCallback = std::function<void(const std::string& file, uint32_t line)>;
    void SetNavigateCallback(NavigateCallback cb);
    void NavigateTo(size_t errorIndex);

    size_t ErrorCount()   const;
    size_t WarningCount() const;
    bool   HasErrors()    const;

    void SetFilter(ErrorSeverity minSeverity);
    std::vector<ParsedError> GetFiltered() const;

private:
    bool ParseLine(const std::string& line, ParsedError& out) const;

    std::vector<ParsedError> m_errors;
    std::vector<ParsedError> m_warnings;
    NavigateCallback         m_navigateCb;
    ErrorSeverity            m_filterMin = ErrorSeverity::Info;
    bool                     m_visible   = true;
};

} // namespace Editor
