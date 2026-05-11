#pragma once

#include <gradiumpp/types.hpp>
#include <gradiumpp/transport/http_transport.hpp>

#include <memory>
#include <string>

namespace gradium {

/// Client for Gradium usage/credits endpoint (GET /usages/credits).
class CreditsClient {
public:
    explicit CreditsClient(
        std::string api_key,
        std::shared_ptr<transport::IHttpTransport> http_transport = nullptr,
        std::string base_url = gradium::api::base_url);

    std::string    getCredits();
    CreditsSummary getCreditsTyped();

private:
    std::string _apiKey;
    std::string _baseUrl;
    std::shared_ptr<transport::IHttpTransport> _httpTransport;
};

} // namespace gradium
