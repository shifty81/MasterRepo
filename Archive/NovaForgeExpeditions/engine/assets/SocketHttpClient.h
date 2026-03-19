#pragma once
#include "HttpClient.h"
#include <string>
#include <cstdint>

namespace atlas::asset {

/// Configuration for the socket-based HTTP client
struct SocketHttpConfig {
    uint32_t connectTimeoutMs = 5000;
    uint32_t readTimeoutMs = 10000;
    std::string userAgent = "AtlasEngine/1.0";
    uint32_t maxRedirects = 5;
    size_t maxResponseSizeBytes = 50 * 1024 * 1024; // 50 MB
};

/// Production HTTP client using POSIX sockets.
/// Supports HTTP GET and file download over TCP.
/// Note: HTTPS requires external TLS library integration.
class SocketHttpClient : public IHttpClient {
public:
    explicit SocketHttpClient(const SocketHttpConfig& config = {});
    ~SocketHttpClient() override = default;

    HttpResponse Get(
        const std::string& url,
        const std::vector<std::pair<std::string, std::string>>& headers = {}) override;

    HttpResponse Post(
        const std::string& url,
        const std::string& body,
        const std::vector<std::pair<std::string, std::string>>& headers = {}) override;

    HttpResponse DownloadFile(
        const std::string& url,
        const std::string& outputPath,
        const std::vector<std::pair<std::string, std::string>>& headers = {}) override;

    const SocketHttpConfig& GetConfig() const;
    void SetConfig(const SocketHttpConfig& config);

    /// Parse a URL into host, port, and path components.
    /// Returns false if the URL is malformed.
    static bool ParseURL(const std::string& url,
                         std::string& outHost,
                         uint16_t& outPort,
                         std::string& outPath);

    /// Tracks total requests made (for diagnostics)
    uint32_t TotalRequestCount() const;

private:
    SocketHttpConfig m_config;
    uint32_t m_requestCount = 0;

    /// Internal: perform raw HTTP GET and return response
    HttpResponse DoGet(const std::string& host, uint16_t port,
                       const std::string& path,
                       const std::vector<std::pair<std::string, std::string>>& headers);

    /// Internal: perform raw HTTP POST and return response
    HttpResponse DoPost(const std::string& host, uint16_t port,
                        const std::string& path,
                        const std::string& body,
                        const std::vector<std::pair<std::string, std::string>>& headers);
};

} // namespace atlas::asset
