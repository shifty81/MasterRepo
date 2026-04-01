#pragma once
#include <string>
#include <unordered_map>

namespace NF {

/// @brief Response object returned by HTTP requests.
struct HttpResponse {
    int StatusCode{0};                                    ///< HTTP status code (e.g. 200).
    std::string Body;                                     ///< Response body text.
    std::unordered_map<std::string, std::string> Headers; ///< Response headers.
};

/// @brief Stub HTTP/1.1 client for simple GET and POST requests.
class HttpClient {
public:
    /// @brief Perform a blocking HTTP GET request.
    /// @param url  Full URL to request (e.g. "http://example.com/api").
    /// @return Populated HttpResponse.
    [[nodiscard]] HttpResponse Get(const std::string& url);

    /// @brief Perform a blocking HTTP POST request.
    /// @param url   Full URL to post to.
    /// @param body  Request body string.
    /// @return Populated HttpResponse.
    [[nodiscard]] HttpResponse Post(const std::string& url, const std::string& body);
};

} // namespace NF
