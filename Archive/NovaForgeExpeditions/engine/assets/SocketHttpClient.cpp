#include "SocketHttpClient.h"
#include "../core/Logger.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <cstring>

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cerrno>
#include <sys/time.h>
#endif

namespace atlas::asset {

SocketHttpClient::SocketHttpClient(const SocketHttpConfig& config)
    : m_config(config) {}

const SocketHttpConfig& SocketHttpClient::GetConfig() const {
    return m_config;
}

void SocketHttpClient::SetConfig(const SocketHttpConfig& config) {
    m_config = config;
}

uint32_t SocketHttpClient::TotalRequestCount() const {
    return m_requestCount;
}

bool SocketHttpClient::ParseURL(const std::string& url,
                                std::string& outHost,
                                uint16_t& outPort,
                                std::string& outPath) {
    // Reject https
    if (url.rfind("https://", 0) == 0) {
        return false;
    }

    std::string remainder;
    if (url.rfind("http://", 0) == 0) {
        remainder = url.substr(7);
    } else {
        return false;
    }

    if (remainder.empty()) return false;

    // Split host:port from path
    size_t slashPos = remainder.find('/');
    std::string hostPort;
    if (slashPos == std::string::npos) {
        hostPort = remainder;
        outPath = "/";
    } else {
        hostPort = remainder.substr(0, slashPos);
        outPath = remainder.substr(slashPos);
    }

    if (hostPort.empty()) return false;

    // Split host and port
    size_t colonPos = hostPort.find(':');
    if (colonPos == std::string::npos) {
        outHost = hostPort;
        outPort = 80;
    } else {
        outHost = hostPort.substr(0, colonPos);
        std::string portStr = hostPort.substr(colonPos + 1);
        if (portStr.empty()) return false;
        try {
            int port = std::stoi(portStr);
            if (port <= 0 || port > 65535) return false;
            outPort = static_cast<uint16_t>(port);
        } catch (...) {
            return false;
        }
    }

    if (outHost.empty()) return false;
    return true;
}

HttpResponse SocketHttpClient::Get(
    const std::string& url,
    const std::vector<std::pair<std::string, std::string>>& headers) {

    std::string host, path;
    uint16_t port = 80;

    if (!ParseURL(url, host, port, path)) {
        HttpResponse resp;
        resp.statusCode = 0;
        resp.errorMessage = "Invalid or unsupported URL: " + url;
        return resp;
    }

    ++m_requestCount;
    atlas::Logger::Info("[SocketHttpClient] GET " + url);
    return DoGet(host, port, path, headers);
}

HttpResponse SocketHttpClient::DownloadFile(
    const std::string& url,
    const std::string& outputPath,
    const std::vector<std::pair<std::string, std::string>>& headers) {

    auto resp = Get(url, headers);
    if (!resp.IsSuccess()) return resp;

    // Create parent directories if needed
    auto parentPath = std::filesystem::path(outputPath).parent_path();
    if (!parentPath.empty()) {
        std::filesystem::create_directories(parentPath);
    }

    std::ofstream out(outputPath, std::ios::binary);
    if (!out.is_open()) {
        resp.statusCode = 0;
        resp.errorMessage = "Failed to open output file: " + outputPath;
        return resp;
    }

    out.write(resp.body.data(), static_cast<std::streamsize>(resp.body.size()));
    out.close();

    atlas::Logger::Info("[SocketHttpClient] Downloaded to " + outputPath);
    return resp;
}

HttpResponse SocketHttpClient::Post(
    const std::string& url,
    const std::string& body,
    const std::vector<std::pair<std::string, std::string>>& headers) {

    std::string host, path;
    uint16_t port = 80;

    if (!ParseURL(url, host, port, path)) {
        HttpResponse resp;
        resp.statusCode = 0;
        resp.errorMessage = "Invalid or unsupported URL: " + url;
        return resp;
    }

    ++m_requestCount;
    atlas::Logger::Info("[SocketHttpClient] POST " + url);
    return DoPost(host, port, path, body, headers);
}

HttpResponse SocketHttpClient::DoPost(
    const std::string& host, uint16_t port,
    const std::string& path,
    const std::string& body,
    const std::vector<std::pair<std::string, std::string>>& headers) {

    HttpResponse resp;

#ifndef _WIN32
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        resp.statusCode = 0;
        resp.errorMessage = "Failed to create socket";
        return resp;
    }

    // Set connect and read timeouts
    struct timeval tv{};
    tv.tv_sec = static_cast<time_t>(m_config.connectTimeoutMs / 1000);
    tv.tv_usec = static_cast<suseconds_t>((m_config.connectTimeoutMs % 1000) * 1000);
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    struct timeval rtv{};
    rtv.tv_sec = static_cast<time_t>(m_config.readTimeoutMs / 1000);
    rtv.tv_usec = static_cast<suseconds_t>((m_config.readTimeoutMs % 1000) * 1000);
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &rtv, sizeof(rtv));

    // Resolve host using getaddrinfo (thread-safe, IPv4/IPv6 aware)
    struct addrinfo hints{}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    std::string portStr = std::to_string(port);
    int gai = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result);
    if (gai != 0 || !result) {
        close(sockfd);
        resp.statusCode = 0;
        resp.errorMessage = "Failed to resolve host: " + host;
        return resp;
    }

    if (connect(sockfd, result->ai_addr, result->ai_addrlen) < 0) {
        freeaddrinfo(result);
        close(sockfd);
        resp.statusCode = 0;
        resp.errorMessage = "Connection refused: " + host + ":" + std::to_string(port);
        return resp;
    }
    freeaddrinfo(result);

    // Build HTTP POST request
    std::string request = "POST " + path + " HTTP/1.1\r\n";
    request += "Host: " + host + "\r\n";
    request += "User-Agent: " + m_config.userAgent + "\r\n";
    request += "Connection: close\r\n";
    request += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    for (const auto& [key, value] : headers) {
        request += key + ": " + value + "\r\n";
    }
    request += "\r\n";
    request += body;

    ssize_t sent = send(sockfd, request.c_str(), request.size(), 0);
    if (sent < 0) {
        close(sockfd);
        resp.statusCode = 0;
        resp.errorMessage = "Failed to send request";
        return resp;
    }

    // Read response
    std::string rawResponse;
    char buf[4096];
    while (true) {
        ssize_t n = recv(sockfd, buf, sizeof(buf), 0);
        if (n <= 0) break;
        rawResponse.append(buf, static_cast<size_t>(n));
        if (rawResponse.size() > m_config.maxResponseSizeBytes) {
            atlas::Logger::Info("[SocketHttpClient] Response truncated at " +
                                std::to_string(m_config.maxResponseSizeBytes) + " bytes");
            break;
        }
    }
    close(sockfd);

    // Parse status line
    size_t statusEnd = rawResponse.find("\r\n");
    if (statusEnd == std::string::npos) {
        resp.statusCode = 0;
        resp.errorMessage = "Invalid HTTP response";
        return resp;
    }

    std::string statusLine = rawResponse.substr(0, statusEnd);
    size_t spacePos = statusLine.find(' ');
    if (spacePos != std::string::npos) {
        std::string codeStr = statusLine.substr(spacePos + 1, 3);
        try { resp.statusCode = std::stoi(codeStr); } catch (...) { resp.statusCode = 0; }
    }

    // Find body after \r\n\r\n
    size_t headerEnd = rawResponse.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        resp.body = rawResponse.substr(headerEnd + 4);
    }
#else
    resp.statusCode = 0;
    resp.errorMessage = "Socket HTTP not supported on Windows";
#endif

    return resp;
}

HttpResponse SocketHttpClient::DoGet(
    const std::string& host, uint16_t port,
    const std::string& path,
    const std::vector<std::pair<std::string, std::string>>& headers) {

    HttpResponse resp;

#ifndef _WIN32
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        resp.statusCode = 0;
        resp.errorMessage = "Failed to create socket";
        return resp;
    }

    // Set connect and read timeouts
    struct timeval tv{};
    tv.tv_sec = static_cast<time_t>(m_config.connectTimeoutMs / 1000);
    tv.tv_usec = static_cast<suseconds_t>((m_config.connectTimeoutMs % 1000) * 1000);
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    struct timeval rtv{};
    rtv.tv_sec = static_cast<time_t>(m_config.readTimeoutMs / 1000);
    rtv.tv_usec = static_cast<suseconds_t>((m_config.readTimeoutMs % 1000) * 1000);
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &rtv, sizeof(rtv));

    // Resolve host using getaddrinfo (thread-safe, IPv4/IPv6 aware)
    struct addrinfo hints{}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    std::string portStr = std::to_string(port);
    int gai = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result);
    if (gai != 0 || !result) {
        close(sockfd);
        resp.statusCode = 0;
        resp.errorMessage = "Failed to resolve host: " + host;
        return resp;
    }

    if (connect(sockfd, result->ai_addr, result->ai_addrlen) < 0) {
        freeaddrinfo(result);
        close(sockfd);
        resp.statusCode = 0;
        resp.errorMessage = "Connection refused: " + host + ":" + std::to_string(port);
        return resp;
    }
    freeaddrinfo(result);

    // Build HTTP request
    std::string request = "GET " + path + " HTTP/1.1\r\n";
    request += "Host: " + host + "\r\n";
    request += "User-Agent: " + m_config.userAgent + "\r\n";
    request += "Connection: close\r\n";
    for (const auto& [key, value] : headers) {
        request += key + ": " + value + "\r\n";
    }
    request += "\r\n";

    ssize_t sent = send(sockfd, request.c_str(), request.size(), 0);
    if (sent < 0) {
        close(sockfd);
        resp.statusCode = 0;
        resp.errorMessage = "Failed to send request";
        return resp;
    }

    // Read response
    std::string rawResponse;
    char buf[4096];
    while (true) {
        ssize_t n = recv(sockfd, buf, sizeof(buf), 0);
        if (n <= 0) break;
        rawResponse.append(buf, static_cast<size_t>(n));
        if (rawResponse.size() > m_config.maxResponseSizeBytes) {
            atlas::Logger::Info("[SocketHttpClient] Response truncated at " +
                                std::to_string(m_config.maxResponseSizeBytes) + " bytes");
            break;
        }
    }
    close(sockfd);

    // Parse status line
    size_t statusEnd = rawResponse.find("\r\n");
    if (statusEnd == std::string::npos) {
        resp.statusCode = 0;
        resp.errorMessage = "Invalid HTTP response";
        return resp;
    }

    std::string statusLine = rawResponse.substr(0, statusEnd);
    // "HTTP/1.1 200 OK"
    size_t spacePos = statusLine.find(' ');
    if (spacePos != std::string::npos) {
        std::string codeStr = statusLine.substr(spacePos + 1, 3);
        try { resp.statusCode = std::stoi(codeStr); } catch (...) { resp.statusCode = 0; }
    }

    // Find body after \r\n\r\n
    size_t headerEnd = rawResponse.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        resp.body = rawResponse.substr(headerEnd + 4);
    }
#else
    resp.statusCode = 0;
    resp.errorMessage = "Socket HTTP not supported on Windows";
#endif

    return resp;
}

} // namespace atlas::asset
