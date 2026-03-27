#include "Core/Net/Http/HttpClient/HttpClient.h"
#include <algorithm>
#include <deque>
#include <sstream>
#include <vector>

// NOTE: This is a stub/simulation implementation. Real network I/O would use
// platform sockets or libcurl. Here we return stub responses so the API compiles
// and links on any platform without external dependencies.

namespace Core {

struct AsyncRequest {
    RequestHandle handle;
    HttpRequest   req;
    HttpCallback  cb;
    float         elapsed{0.f};
    bool          done{false};
    HttpResponse  response;
};

struct HttpClient::Impl {
    std::unordered_map<std::string,std::string> defaultHeaders;
    std::deque<AsyncRequest> pending;
    uint32_t maxConcurrent{8};
    uint32_t defaultTimeout{5000};
    uint64_t nextHandle{1};
    std::string proxyHost;
    uint16_t    proxyPort{0};

    HttpResponse BuildStubResponse(const HttpRequest& req) const {
        HttpResponse r;
        r.statusCode=200;
        r.body="{\"stub\":true,\"url\":\""+req.url+"\"}";
        r.headers["Content-Type"]="application/json";
        return r;
    }
};

HttpClient::HttpClient()  : m_impl(new Impl){}
HttpClient::~HttpClient() { Shutdown(); delete m_impl; }
void HttpClient::Init()     {}
void HttpClient::Shutdown() { m_impl->pending.clear(); }

void HttpClient::SetDefaultHeaders(const std::unordered_map<std::string,std::string>& h){
    m_impl->defaultHeaders=h;
}
void HttpClient::SetDefaultTimeout(uint32_t ms){ m_impl->defaultTimeout=ms; }
void HttpClient::SetProxy(const std::string& host, uint16_t port){
    m_impl->proxyHost=host; m_impl->proxyPort=port;
}
void HttpClient::SetMaxConcurrent(uint32_t n){ m_impl->maxConcurrent=n; }

HttpResponse HttpClient::Get(const std::string& url,
                               const std::unordered_map<std::string,std::string>& headers){
    HttpRequest req; req.method="GET"; req.url=url; req.headers=headers;
    return Send(req);
}
HttpResponse HttpClient::Post(const std::string& url, const std::string& body,
                                const std::unordered_map<std::string,std::string>& headers){
    HttpRequest req; req.method="POST"; req.url=url; req.body=body; req.headers=headers;
    return Send(req);
}
HttpResponse HttpClient::Send(const HttpRequest& req){
    // Merge default headers
    HttpRequest merged=req;
    for(auto& [k,v]:m_impl->defaultHeaders) if(!merged.headers.count(k)) merged.headers[k]=v;
    return m_impl->BuildStubResponse(merged);
}

RequestHandle HttpClient::GetAsync(const std::string& url, HttpCallback cb,
                                    const std::unordered_map<std::string,std::string>& headers){
    HttpRequest req; req.method="GET"; req.url=url; req.headers=headers;
    return SendAsync(req,cb);
}
RequestHandle HttpClient::PostAsync(const std::string& url, const std::string& body,
                                     HttpCallback cb,
                                     const std::unordered_map<std::string,std::string>& headers){
    HttpRequest req; req.method="POST"; req.url=url; req.body=body; req.headers=headers;
    return SendAsync(req,cb);
}
RequestHandle HttpClient::SendAsync(const HttpRequest& req, HttpCallback cb){
    AsyncRequest ar; ar.handle=m_impl->nextHandle++; ar.req=req; ar.cb=cb;
    ar.response=m_impl->BuildStubResponse(req);
    m_impl->pending.push_back(ar);
    return ar.handle;
}

void HttpClient::CancelRequest(RequestHandle handle){
    auto& v=m_impl->pending;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& r){return r.handle==handle;}),v.end());
}
bool HttpClient::IsRequestPending(RequestHandle handle) const {
    for(auto& r:m_impl->pending) if(r.handle==handle) return true; return false;
}

void HttpClient::Tick(float /*dt*/){
    // Immediately complete stub requests
    while(!m_impl->pending.empty()){
        auto req=m_impl->pending.front(); m_impl->pending.pop_front();
        if(req.cb) req.cb(req.response);
    }
}

} // namespace Core
