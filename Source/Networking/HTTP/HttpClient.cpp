#include "Networking/HTTP/HttpClient.h"

namespace NF {

HttpResponse HttpClient::Get(const std::string& /*url*/) {
    // Stub: a real implementation would open a socket, send an HTTP GET
    // request, parse the response headers and body, then return them.
    HttpResponse response;
    response.StatusCode = 200;
    response.Body       = "";
    response.Headers["Content-Type"] = "text/plain";
    return response;
}

HttpResponse HttpClient::Post(const std::string& /*url*/, const std::string& /*body*/) {
    // Stub: a real implementation would open a socket, send an HTTP POST
    // request with the provided body, then parse and return the response.
    HttpResponse response;
    response.StatusCode = 200;
    response.Body       = "";
    response.Headers["Content-Type"] = "text/plain";
    return response;
}

} // namespace NF
