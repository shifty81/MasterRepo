#pragma once
/**
 * @file HttpClient.h
 * @brief Simple synchronous + async GET/POST, header map, body, timeout, callback.
 *
 * Features:
 *   - HttpRequest: method, url, headers map, body string, timeoutMs
 *   - HttpResponse: statusCode, headers, body, error string
 *   - Synchronous: Get(url) / Post(url, body) → HttpResponse
 *   - Async: GetAsync / PostAsync → returns RequestHandle, fires callback on completion
 *   - CancelRequest(handle)
 *   - Tick: drives async completion (poll-based, no threads required on caller side)
 *   - SetDefaultHeaders: applied to every request (e.g. User-Agent, Auth token)
 *   - Response body as string or binary (vector<uint8_t>)
 *   - SetProxy(host, port)
 *   - Max concurrent async requests limit
 *
 * Typical usage:
 * @code
 *   HttpClient http;
 *   http.Init();
 *   auto res = http.Get("http://localhost:8080/api/status");
 *   if (res.statusCode == 200) Log(res.body);
 *
 *   http.PostAsync("http://api/ingest", body, [](const HttpResponse& r){
 *       Log(r.statusCode);
 *   });
 *   http.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Core {

struct HttpRequest {
    std::string method{"GET"};
    std::string url;
    std::unordered_map<std::string,std::string> headers;
    std::string body;
    uint32_t    timeoutMs{5000};
};

struct HttpResponse {
    int         statusCode{0};
    std::unordered_map<std::string,std::string> headers;
    std::string body;
    std::string error;
    bool        success() const { return statusCode>=200&&statusCode<300; }
};

using HttpCallback    = std::function<void(const HttpResponse&)>;
using RequestHandle   = uint64_t;

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);      ///< drives async completion

    // Synchronous
    HttpResponse Get (const std::string& url,
                       const std::unordered_map<std::string,std::string>& headers={});
    HttpResponse Post(const std::string& url, const std::string& body,
                       const std::unordered_map<std::string,std::string>& headers={});
    HttpResponse Send(const HttpRequest& req);

    // Asynchronous
    RequestHandle GetAsync (const std::string& url, HttpCallback cb,
                             const std::unordered_map<std::string,std::string>& headers={});
    RequestHandle PostAsync(const std::string& url, const std::string& body,
                             HttpCallback cb,
                             const std::unordered_map<std::string,std::string>& headers={});
    RequestHandle SendAsync(const HttpRequest& req, HttpCallback cb);
    void          CancelRequest(RequestHandle handle);
    bool          IsRequestPending(RequestHandle handle) const;

    // Defaults
    void SetDefaultHeaders(const std::unordered_map<std::string,std::string>& h);
    void SetDefaultTimeout(uint32_t ms);
    void SetProxy         (const std::string& host, uint16_t port);
    void SetMaxConcurrent (uint32_t n);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Core
