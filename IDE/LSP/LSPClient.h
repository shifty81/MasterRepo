#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace IDE {

// ──────────────────────────────────────────────────────────────
// LSP basic types (0-based positions)
// ──────────────────────────────────────────────────────────────

struct LSPPosition {
    uint32_t line      = 0;
    uint32_t character = 0;
};

struct LSPRange {
    LSPPosition start;
    LSPPosition end;
};

struct LSPLocation {
    std::string uri;
    LSPRange    range;
};

// ──────────────────────────────────────────────────────────────
// Diagnostics
// ──────────────────────────────────────────────────────────────

enum class LSPDiagnosticSeverity { Error = 1, Warning = 2, Info = 3, Hint = 4 };

struct LSPDiagnostic {
    LSPRange               range;
    LSPDiagnosticSeverity  severity = LSPDiagnosticSeverity::Error;
    std::string            message;
    std::string            code;
    std::string            source;
};

// ──────────────────────────────────────────────────────────────
// Completion
// ──────────────────────────────────────────────────────────────

struct LSPCompletionItem {
    std::string label;
    std::string detail;
    std::string insertText;
    uint32_t    kind       = 0;   // LSP CompletionItemKind
    bool        deprecated = false;
};

// ──────────────────────────────────────────────────────────────
// Hover
// ──────────────────────────────────────────────────────────────

struct LSPHover {
    std::string contents;  // markdown
    LSPRange    range;
    bool        valid = false;
};

// ──────────────────────────────────────────────────────────────
// Symbol
// ──────────────────────────────────────────────────────────────

struct LSPSymbol {
    std::string name;
    uint32_t    kind = 0;
    LSPLocation location;
    std::string containerName;
};

// ──────────────────────────────────────────────────────────────
// LSPClient — JSON-RPC 2.0 pipe-based language server client
// ──────────────────────────────────────────────────────────────

class LSPClient {
public:
    // Lifecycle
    bool Start(const std::string& serverExecutable,
               const std::vector<std::string>& args = {});
    void Shutdown();
    bool IsRunning() const;

    // Notifications to server (fire-and-forget)
    void NotifyOpen  (const std::string& uri, const std::string& languageId,
                      const std::string& text);
    void NotifyChange(const std::string& uri, const std::string& newText);
    void NotifyClose (const std::string& uri);

    // Synchronous requests (blocking, with internal timeout)
    std::vector<LSPCompletionItem> RequestCompletion    (const std::string& uri, LSPPosition pos);
    LSPHover                       RequestHover         (const std::string& uri, LSPPosition pos);
    std::vector<LSPLocation>       RequestDefinition    (const std::string& uri, LSPPosition pos);
    std::vector<LSPLocation>       RequestReferences    (const std::string& uri, LSPPosition pos);
    std::vector<LSPSymbol>         RequestDocumentSymbols(const std::string& uri);
    std::vector<LSPDiagnostic>     GetDiagnostics       (const std::string& uri) const;

    // Diagnostic callback (invoked when server pushes textDocument/publishDiagnostics)
    using DiagnosticCallback =
        std::function<void(const std::string& uri, const std::vector<LSPDiagnostic>&)>;
    void SetDiagnosticCallback(DiagnosticCallback cb);

    // Server capability queries (valid after Start succeeds)
    bool HasCompletionSupport() const;
    bool HasHoverSupport()      const;
    bool HasGotoDefinition()    const;

    // Server name reported in initialize response
    const std::string& ServerName() const;

private:
    // Low-level JSON-RPC transport
    void        SendRequest     (uint32_t id, const std::string& method,
                                 const std::string& paramsJson);
    void        SendNotification(const std::string& method, const std::string& paramsJson);
    std::string ReadResponse    ();

    // Minimal JSON helpers (no external lib)
    static std::string EscapeJson (const std::string& s);
    static std::string MakePosition(LSPPosition pos);
    static std::string StringBetween(const std::string& src,
                                      const std::string& open,
                                      const std::string& close,
                                      size_t startPos = 0);

    // Process handles
    FILE* m_childIn  = nullptr;   // write  → server stdin
    FILE* m_childOut = nullptr;   // read   ← server stdout
    int   m_childPid = -1;

    bool        m_running    = false;
    std::string m_serverName;

    DiagnosticCallback m_diagCb;
    std::unordered_map<std::string, std::vector<LSPDiagnostic>> m_diagnostics;

    uint32_t m_nextId      = 1;
    bool     m_hasCompletion = false;
    bool     m_hasHover      = false;
    bool     m_hasDefinition = false;
};

} // namespace IDE
