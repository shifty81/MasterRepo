#include "AI/OllamaClient/OllamaClient.h"
#include "Engine/Core/Logger.h"
#include <cstring>
#include <sstream>
#include <algorithm>

// Platform socket headers (POSIX / Winsock)
#if defined(_WIN32)
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "Ws2_32.lib")
   using SockLen = int;
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <netdb.h>
#  include <unistd.h>
#  include <arpa/inet.h>
   using SockLen = socklen_t;
#  define INVALID_SOCKET (-1)
#  define closesocket    close
#endif

namespace AI {

OllamaClient::OllamaClient(const std::string& host, uint16_t port)
    : m_host(host), m_port(port) {}

// ── Simple JSON field extractor ───────────────────────────────────────────
std::string OllamaClient::ExtractField(const std::string& json,
                                       const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return {};
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return {};
    ++pos;
    while (pos < json.size() && json[pos] == ' ') ++pos;
    if (pos >= json.size()) return {};

    if (json[pos] == '"') {
        // string value
        auto start = pos + 1;
        auto end   = json.find('"', start);
        if (end == std::string::npos) return {};
        // Handle JSON escape sequences properly
        std::string out;
        for (size_t i = start; i < end; ++i) {
            if (json[i] == '\\' && i + 1 < end) {
                ++i;
                switch (json[i]) {
                    case '"':  out += '"';  break;
                    case '\\': out += '\\'; break;
                    case 'n':  out += '\n'; break;
                    case 'r':  out += '\r'; break;
                    case 't':  out += '\t'; break;
                    default:   out += json[i]; break;
                }
            } else {
                out += json[i];
            }
        }
        return out;
    } else {
        // number / bool / null value — take until comma or }
        auto end = json.find_first_of(",}\n", pos);
        auto val = json.substr(pos, end != std::string::npos ? end - pos : std::string::npos);
        // trim whitespace
        while (!val.empty() && (val.back() == ' ' || val.back() == '\r')) val.pop_back();
        return val;
    }
}

// ── Raw TCP helper ────────────────────────────────────────────────────────
std::string OllamaClient::HttpPost(const std::string& path,
                                   const std::string& body) {
#if defined(_WIN32)
    WSADATA wd;
    WSAStartup(MAKEWORD(2,2), &wd);
#endif
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    std::string portStr = std::to_string(m_port);

    if (getaddrinfo(m_host.c_str(), portStr.c_str(), &hints, &res) != 0) {
        return "";
    }

    int sock = (int)socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == INVALID_SOCKET) { freeaddrinfo(res); return ""; }

    if (connect(sock, res->ai_addr, (SockLen)res->ai_addrlen) != 0) {
        closesocket(sock); freeaddrinfo(res); return "";
    }
    freeaddrinfo(res);

    // Build HTTP request
    std::ostringstream req;
    req << "POST " << path << " HTTP/1.0\r\n"
        << "Host: " << m_host << ":" << m_port << "\r\n"
        << "Content-Type: application/json\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Connection: close\r\n\r\n"
        << body;
    std::string reqStr = req.str();
    send(sock, reqStr.c_str(), (int)reqStr.size(), 0);

    // Read response
    std::string response;
    char buf[4096];
    int n;
    while ((n = recv(sock, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        response += buf;
    }
    closesocket(sock);

    // Strip HTTP headers — find double CRLF
    auto hdrEnd = response.find("\r\n\r\n");
    if (hdrEnd != std::string::npos)
        return response.substr(hdrEnd + 4);
    return response;
}

std::string OllamaClient::HttpGet(const std::string& path) {
#if defined(_WIN32)
    WSADATA wd; WSAStartup(MAKEWORD(2,2), &wd);
#endif
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    std::string portStr = std::to_string(m_port);
    if (getaddrinfo(m_host.c_str(), portStr.c_str(), &hints, &res) != 0) return "";
    int sock = (int)socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == INVALID_SOCKET) { freeaddrinfo(res); return ""; }
    if (connect(sock, res->ai_addr, (SockLen)res->ai_addrlen) != 0) {
        closesocket(sock); freeaddrinfo(res); return "";
    }
    freeaddrinfo(res);

    std::ostringstream req;
    req << "GET " << path << " HTTP/1.0\r\n"
        << "Host: " << m_host << ":" << m_port << "\r\n"
        << "Connection: close\r\n\r\n";
    std::string reqStr = req.str();
    send(sock, reqStr.c_str(), (int)reqStr.size(), 0);

    std::string response;
    char buf[4096];
    int n;
    while ((n = recv(sock, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0'; response += buf;
    }
    closesocket(sock);

    auto hdrEnd = response.find("\r\n\r\n");
    if (hdrEnd != std::string::npos) return response.substr(hdrEnd + 4);
    return response;
}

// ── AI-LV-01: Ping ───────────────────────────────────────────────────────
bool OllamaClient::Ping() {
    auto resp = HttpGet("/api/tags");
    bool ok = !resp.empty() && resp.find("models") != std::string::npos;
    Engine::Core::Logger::Info(ok
        ? "AI-LV-01: Ollama reachable at " + m_host + ":" + std::to_string(m_port)
        : "AI-LV-01: Ollama not reachable at " + m_host + ":" + std::to_string(m_port));
    return ok;
}

std::string OllamaClient::ListModels() {
    auto resp = HttpGet("/api/tags");
    // Extract model names from JSON array
    std::string out;
    size_t pos = 0;
    while ((pos = resp.find("\"name\":", pos)) != std::string::npos) {
        auto name = ExtractField(resp.substr(pos), "name");
        if (!name.empty()) { if (!out.empty()) out += ", "; out += name; }
        pos += 7;
    }
    return out;
}

// ── JSON string escape helper (prevents injection in /api/generate body) ──
static std::string JsonEscapeStr(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
                break;
        }
    }
    return out;
}

// ── AI-LV-02/03/04: Generate ─────────────────────────────────────────────
OllamaResponse OllamaClient::Generate(const std::string& model,
                                       const std::string& prompt,
                                       const std::string& system) {
    // Build /api/generate JSON body (stream:false for blocking call)
    std::ostringstream body;
    body << "{\"model\":\""  << JsonEscapeStr(model)  << "\""
         << ",\"prompt\":\"" << JsonEscapeStr(prompt) << "\"";
    if (!system.empty())
        body << ",\"system\":\"" << JsonEscapeStr(system) << "\"";
    body << ",\"stream\":false}";

    auto raw = HttpPost("/api/generate", body.str());
    OllamaResponse r;
    if (raw.empty()) {
        r.error = "No response from Ollama";
        return r;
    }
    r.text       = ExtractField(raw, "response");
    r.httpStatus = 200;
    r.success    = !r.text.empty();
    if (!r.success) {
        std::string apiError = ExtractField(raw, "error");
        r.error = apiError.empty()
            ? "Empty response field — raw: " + raw.substr(0, 200)
            : apiError;
    }

    if (m_streamCb) m_streamCb(r.text, true);
    return r;
}

// AI-LV-02
OllamaResponse OllamaClient::GenerateCode(const std::string& task,
                                           const std::string& context) {
    std::string sys = "You are AtlasForge AI, an expert C++20 game-engine programmer. "
                      "Output ONLY compilable C++ code, no explanations.";
    std::string prompt = task;
    if (!context.empty()) prompt += "\n\nContext:\n" + context;
    auto r = Generate(m_model, prompt, sys);
    if (r.success)
        Engine::Core::Logger::Info("AI-LV-02: Code generation complete (" +
            std::to_string(r.text.size()) + " chars)");
    return r;
}

// AI-LV-03
OllamaResponse OllamaClient::FixBuildError(const std::string& errorText,
                                            const std::string& sourceSnippet) {
    std::string sys = "You are AtlasForge AI build-error assistant. "
                      "Given a compiler error and the relevant source, output a minimal patch "
                      "as a unified diff or the corrected code block. No explanations.";
    std::string prompt = "Build error:\n" + errorText;
    if (!sourceSnippet.empty())
        prompt += "\n\nRelevant source:\n" + sourceSnippet;
    auto r = Generate(m_model, prompt, sys);
    if (r.success)
        Engine::Core::Logger::Info("AI-LV-03: Build-error fix suggestion received");
    return r;
}

// AI-LV-04
OllamaResponse OllamaClient::PCGPromptToScene(const std::string& description) {
    std::string sys = "You are AtlasForge PCG assistant. "
                      "Translate the user's description into a compact JSON object with fields: "
                      "type (Building/Tower/Ruin/Bunker/Outpost), seed (int), "
                      "sizeX, sizeY, sizeZ, floors (all ints), addDoors (bool). "
                      "Output ONLY the JSON object, no prose.";
    auto r = Generate(m_model, description, sys);
    if (r.success)
        Engine::Core::Logger::Info("AI-LV-04: PCG prompt processed → " + r.text.substr(0,80));
    return r;
}

} // namespace AI
