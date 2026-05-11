#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace gradium::transport {

enum class HttpMethod { Get, Post, Put, Delete };

struct MultipartFile {
    std::string field_name;
    std::string file_path;
    std::string content_type; ///< empty = auto-detect from extension
};

struct MultipartField {
    std::string field_name;
    std::string value;
    std::string content_type; ///< empty = default text/plain handling
};

struct HttpRequest {
    HttpMethod  method{HttpMethod::Get};
    std::string url;
    std::map<std::string, std::string> headers;
    std::string content_type;
    std::string body;                          ///< JSON or text body
    std::vector<std::uint8_t> binary_body;     ///< raw audio bytes (ASR REST POST)
    std::vector<MultipartFile> multipart_files; ///< mutually exclusive with body/binary_body
    std::vector<MultipartField> multipart_fields;
    long timeout_ms{0};
};

struct HttpResponse {
    long status_code{0};
    std::map<std::string, std::string> headers;
    std::vector<std::uint8_t> body;
};

/// Synchronous HTTP transport interface. Not required to be thread-safe.
class IHttpTransport {
public:
    virtual ~IHttpTransport() = default;

    /* Throws std::runtime_error on I/O or TLS errors.
       HTTP >= 400 is not thrown — callers check status_code. */
    virtual HttpResponse send(const HttpRequest& request) = 0;
};

} // namespace gradium::transport
