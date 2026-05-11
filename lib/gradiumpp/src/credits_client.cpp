#include <gradiumpp/credits_client.hpp>
#include <gradiumpp/transport/curl_http_transport.hpp>

#include "error_util.hpp"

#include <nlohmann/json.hpp>

namespace gradium {

CreditsClient::CreditsClient(
    std::string api_key,
    std::shared_ptr<transport::IHttpTransport> http_transport,
    std::string base_url)
    : _apiKey(std::move(api_key)),
      _baseUrl(std::move(base_url)),
      _httpTransport(std::move(http_transport))
{
    if (!_httpTransport) {
        _httpTransport = std::make_shared<transport::CurlHttpTransport>();
    }
}

std::string CreditsClient::getCredits()
{
    transport::HttpRequest req;
    req.method = transport::HttpMethod::Get;
    req.url    = _baseUrl + credits::endpoints::base;
    req.headers["x-api-key"]  = _apiKey;
    req.headers["User-Agent"] = gradium::USER_AGENT;

    auto response = _httpTransport->send(req);
    detail::throwIfError(response, "getCredits");

    return std::string(response.body.begin(), response.body.end());
}

CreditsSummary CreditsClient::getCreditsTyped()
{
    const auto payload = nlohmann::json::parse(getCredits());
    CreditsSummary summary;
    summary.remaining_credits  = payload.value("remaining_credits", 0.0);
    summary.allocated_credits  = payload.value("allocated_credits", 0.0);
    summary.billing_period     = detail::stringOrEmpty(payload, "billing_period");
    summary.next_rollover_date = detail::stringOrEmpty(payload, "next_rollover_date");
    summary.plan_name          = detail::stringOrEmpty(payload, "plan_name");
    return summary;
}

} // namespace gradium
