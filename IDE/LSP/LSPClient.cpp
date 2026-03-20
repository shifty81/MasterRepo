#include "IDE/LSP/LSPClient.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <sstream>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <unistd.h>
#endif

namespace IDE {

// ──────────────────────────────────────────────────────────────
// Minimal JSON helpers
// ──────────────────────────────────────────────────────────────

std::string LSPClient::EscapeJson(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:   out += c;      break;
        }
    }
    return out;
}

std::string LSPClient::MakePosition(LSPPosition pos) {
    return "{\"line\":" + std::to_string(pos.line) +
           ",\"character\":" + std::to_string(pos.character) + "}";
}

// Returns text between first occurrence of `open` (after startPos) and next `close`.
std::string LSPClient::StringBetween(const std::string& src,
                                      const std::string& open,
                                      const std::string& close,
                                      size_t startPos) {
    auto a = src.find(open, startPos);
    if (a == std::string::npos) return {};
    a += open.size();
    auto b = src.find(close, a);
    if (b == std::string::npos) return src.substr(a);
    return src.substr(a, b - a);
}

// ──────────────────────────────────────────────────────────────
// Transport: write Content-Length framed message to child stdin
// ──────────────────────────────────────────────────────────────

void LSPClient::SendRequest(uint32_t id, const std::string& method,
                             const std::string& paramsJson) {
    if (!m_running || !m_childIn) return;
    std::string body = "{\"jsonrpc\":\"2.0\",\"id\":" + std::to_string(id) +
                       ",\"method\":\"" + method + "\",\"params\":" + paramsJson + "}";
    std::string frame = "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
    std::fwrite(frame.data(), 1, frame.size(), m_childIn);
    std::fflush(m_childIn);
}

void LSPClient::SendNotification(const std::string& method, const std::string& paramsJson) {
    if (!m_running || !m_childIn) return;
    std::string body = "{\"jsonrpc\":\"2.0\",\"method\":\"" + method +
                       "\",\"params\":" + paramsJson + "}";
    std::string frame = "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
    std::fwrite(frame.data(), 1, frame.size(), m_childIn);
    std::fflush(m_childIn);
}

// Reads one JSON-RPC response from child stdout (blocking).
std::string LSPClient::ReadResponse() {
    if (!m_running || !m_childOut) return {};

    // Read header lines until blank line
    std::string headerBuf;
    int contentLength = 0;
    while (true) {
        char lineBuf[256] = {};
        if (!std::fgets(lineBuf, sizeof(lineBuf), m_childOut)) return {};
        std::string line(lineBuf);
        if (line == "\r\n" || line == "\n") break;
        if (line.find("Content-Length:") != std::string::npos) {
            auto colon = line.find(':');
            contentLength = std::stoi(line.substr(colon + 1));
        }
    }
    if (contentLength <= 0) return {};

    std::string body(static_cast<size_t>(contentLength), '\0');
    size_t read = std::fread(body.data(), 1, static_cast<size_t>(contentLength), m_childOut);
    if (read != static_cast<size_t>(contentLength)) body.resize(read);
    return body;
}

// ──────────────────────────────────────────────────────────────
// Lifecycle
// ──────────────────────────────────────────────────────────────

bool LSPClient::Start(const std::string& serverExecutable,
                      const std::vector<std::string>& args) {
    if (m_running) return true;

#ifndef _WIN32
    // Build argv
    std::vector<const char*> argv;
    argv.push_back(serverExecutable.c_str());
    for (const auto& a : args) argv.push_back(a.c_str());
    argv.push_back(nullptr);

    int stdinPipe[2], stdoutPipe[2];
    if (pipe(stdinPipe) != 0 || pipe(stdoutPipe) != 0) return false;

    pid_t pid = fork();
    if (pid < 0) return false;
    if (pid == 0) {
        // Child
        dup2(stdinPipe[0],  STDIN_FILENO);
        dup2(stdoutPipe[1], STDOUT_FILENO);
        close(stdinPipe[0]); close(stdinPipe[1]);
        close(stdoutPipe[0]); close(stdoutPipe[1]);
        execvp(serverExecutable.c_str(), const_cast<char* const*>(argv.data()));
        _exit(1);
    }
    // Parent
    close(stdinPipe[0]);
    close(stdoutPipe[1]);
    m_childIn  = fdopen(stdinPipe[1],  "w");
    m_childOut = fdopen(stdoutPipe[0], "r");
    m_childPid = static_cast<int>(pid);
    m_running  = true;
#else
    // On Windows: popen in read+write mode is not natively supported.
    // Use a basic popen for read-only stub; real implementation requires CreateProcess.
    std::string cmd = serverExecutable;
    for (const auto& a : args) cmd += " " + a;
    m_childIn  = nullptr;
    m_childOut = _popen(cmd.c_str(), "r");
    m_running  = (m_childOut != nullptr);
#endif

    if (!m_running) return false;

    // Send initialize request
    uint32_t id = m_nextId++;
    std::string initParams =
        "{\"processId\":null,\"clientInfo\":{\"name\":\"AtlasEditor\",\"version\":\"0.1\"},"
        "\"rootUri\":null,"
        "\"capabilities\":{\"textDocument\":{"
            "\"completion\":{\"completionItem\":{\"snippetSupport\":false}},"
            "\"hover\":{},"
            "\"definition\":{},"
            "\"references\":{},"
            "\"documentSymbol\":{}"
        "}}}";
    SendRequest(id, "initialize", initParams);
    std::string resp = ReadResponse();

    // Parse minimal capabilities from response
    if (resp.find("\"completionProvider\"") != std::string::npos) m_hasCompletion = true;
    if (resp.find("\"hoverProvider\"")      != std::string::npos) m_hasHover      = true;
    if (resp.find("\"definitionProvider\"") != std::string::npos) m_hasDefinition = true;

    m_serverName = StringBetween(resp, "\"name\":\"", "\"");

    // Send initialized notification
    SendNotification("initialized", "{}");
    return true;
}

void LSPClient::Shutdown() {
    if (!m_running) return;
    uint32_t id = m_nextId++;
    SendRequest(id, "shutdown", "null");
    ReadResponse();
    SendNotification("exit", "null");

    if (m_childIn)  { std::fclose(m_childIn);  m_childIn  = nullptr; }
    if (m_childOut) { std::fclose(m_childOut); m_childOut = nullptr; }

#ifndef _WIN32
    if (m_childPid > 0) {
        int status = 0;
        waitpid(m_childPid, &status, 0);
        m_childPid = -1;
    }
#endif
    m_running = false;
}

bool LSPClient::IsRunning() const { return m_running; }

// ──────────────────────────────────────────────────────────────
// Notifications
// ──────────────────────────────────────────────────────────────

void LSPClient::NotifyOpen(const std::string& uri, const std::string& languageId,
                            const std::string& text) {
    if (!m_running) return;
    std::string params =
        "{\"textDocument\":{\"uri\":\"" + EscapeJson(uri) + "\","
        "\"languageId\":\"" + EscapeJson(languageId) + "\","
        "\"version\":1,"
        "\"text\":\"" + EscapeJson(text) + "\"}}";
    SendNotification("textDocument/didOpen", params);
}

void LSPClient::NotifyChange(const std::string& uri, const std::string& newText) {
    if (!m_running) return;
    std::string params =
        "{\"textDocument\":{\"uri\":\"" + EscapeJson(uri) + "\",\"version\":2},"
        "\"contentChanges\":[{\"text\":\"" + EscapeJson(newText) + "\"}]}";
    SendNotification("textDocument/didChange", params);
}

void LSPClient::NotifyClose(const std::string& uri) {
    if (!m_running) return;
    std::string params =
        "{\"textDocument\":{\"uri\":\"" + EscapeJson(uri) + "\"}}";
    SendNotification("textDocument/didClose", params);
}

// ──────────────────────────────────────────────────────────────
// Requests
// ──────────────────────────────────────────────────────────────

std::vector<LSPCompletionItem> LSPClient::RequestCompletion(const std::string& uri,
                                                              LSPPosition pos) {
    if (!m_running) return {};
    uint32_t id = m_nextId++;
    std::string params =
        "{\"textDocument\":{\"uri\":\"" + EscapeJson(uri) + "\"},"
        "\"position\":" + MakePosition(pos) + "}";
    SendRequest(id, "textDocument/completion", params);
    std::string resp = ReadResponse();

    std::vector<LSPCompletionItem> items;
    // Parse items array: locate each "label": entry
    size_t pos2 = 0;
    while (true) {
        auto la = resp.find("\"label\":", pos2);
        if (la == std::string::npos) break;
        LSPCompletionItem item;
        item.label      = StringBetween(resp, "\"label\":\"",      "\"", la);
        item.detail     = StringBetween(resp, "\"detail\":\"",     "\"", la);
        item.insertText = StringBetween(resp, "\"insertText\":\"", "\"", la);
        auto kindStr    = StringBetween(resp, "\"kind\":",         ",",  la);
        if (!kindStr.empty()) {
            try { item.kind = static_cast<uint32_t>(std::stoul(kindStr)); } catch (...) {}
        }
        items.push_back(std::move(item));
        pos2 = la + 8;
    }
    return items;
}

LSPHover LSPClient::RequestHover(const std::string& uri, LSPPosition pos) {
    if (!m_running) return {};
    uint32_t id = m_nextId++;
    std::string params =
        "{\"textDocument\":{\"uri\":\"" + EscapeJson(uri) + "\"},"
        "\"position\":" + MakePosition(pos) + "}";
    SendRequest(id, "textDocument/hover", params);
    std::string resp = ReadResponse();

    LSPHover hover;
    if (resp.find("\"result\":null") != std::string::npos) return hover;
    hover.contents = StringBetween(resp, "\"value\":\"", "\"");
    if (hover.contents.empty())
        hover.contents = StringBetween(resp, "\"contents\":\"", "\"");
    hover.valid = !hover.contents.empty();
    return hover;
}

std::vector<LSPLocation> LSPClient::RequestDefinition(const std::string& uri,
                                                        LSPPosition pos) {
    if (!m_running) return {};
    uint32_t id = m_nextId++;
    std::string params =
        "{\"textDocument\":{\"uri\":\"" + EscapeJson(uri) + "\"},"
        "\"position\":" + MakePosition(pos) + "}";
    SendRequest(id, "textDocument/definition", params);
    std::string resp = ReadResponse();

    std::vector<LSPLocation> locs;
    size_t p = 0;
    while (true) {
        auto a = resp.find("\"uri\":", p);
        if (a == std::string::npos) break;
        LSPLocation loc;
        loc.uri = StringBetween(resp, "\"uri\":\"", "\"", a);
        locs.push_back(std::move(loc));
        p = a + 6;
    }
    return locs;
}

std::vector<LSPLocation> LSPClient::RequestReferences(const std::string& uri,
                                                        LSPPosition pos) {
    if (!m_running) return {};
    uint32_t id = m_nextId++;
    std::string params =
        "{\"textDocument\":{\"uri\":\"" + EscapeJson(uri) + "\"},"
        "\"position\":" + MakePosition(pos) + ","
        "\"context\":{\"includeDeclaration\":true}}";
    SendRequest(id, "textDocument/references", params);
    std::string resp = ReadResponse();

    std::vector<LSPLocation> locs;
    size_t p = 0;
    while (true) {
        auto a = resp.find("\"uri\":", p);
        if (a == std::string::npos) break;
        LSPLocation loc;
        loc.uri = StringBetween(resp, "\"uri\":\"", "\"", a);
        locs.push_back(std::move(loc));
        p = a + 6;
    }
    return locs;
}

std::vector<LSPSymbol> LSPClient::RequestDocumentSymbols(const std::string& uri) {
    if (!m_running) return {};
    uint32_t id = m_nextId++;
    std::string params =
        "{\"textDocument\":{\"uri\":\"" + EscapeJson(uri) + "\"}}";
    SendRequest(id, "textDocument/documentSymbol", params);
    std::string resp = ReadResponse();

    std::vector<LSPSymbol> syms;
    size_t p = 0;
    while (true) {
        auto a = resp.find("\"name\":", p);
        if (a == std::string::npos) break;
        LSPSymbol sym;
        sym.name          = StringBetween(resp, "\"name\":\"",          "\"", a);
        sym.containerName = StringBetween(resp, "\"containerName\":\"", "\"", a);
        auto kindStr      = StringBetween(resp, "\"kind\":",            ",",  a);
        if (!kindStr.empty()) {
            try { sym.kind = static_cast<uint32_t>(std::stoul(kindStr)); } catch (...) {}
        }
        syms.push_back(std::move(sym));
        p = a + 7;
    }
    return syms;
}

std::vector<LSPDiagnostic> LSPClient::GetDiagnostics(const std::string& uri) const {
    auto it = m_diagnostics.find(uri);
    if (it == m_diagnostics.end()) return {};
    return it->second;
}

// ──────────────────────────────────────────────────────────────
// Callback & capabilities
// ──────────────────────────────────────────────────────────────

void LSPClient::SetDiagnosticCallback(DiagnosticCallback cb) {
    m_diagCb = std::move(cb);
}

bool LSPClient::HasCompletionSupport() const { return m_hasCompletion; }
bool LSPClient::HasHoverSupport()      const { return m_hasHover; }
bool LSPClient::HasGotoDefinition()    const { return m_hasDefinition; }

const std::string& LSPClient::ServerName() const { return m_serverName; }

} // namespace IDE
