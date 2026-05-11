#pragma once

#include <gradiumpp/transport/http_transport.hpp>

namespace gradium::transport {

class CurlHttpTransport final : public IHttpTransport {
public:
    HttpResponse send(const HttpRequest& request) override;
};

} // namespace gradium::transport
