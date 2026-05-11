#include <gradiumpp/pronunciation_client.hpp>
#include <gradiumpp/transport/curl_http_transport.hpp>

#include "error_util.hpp"

#include <nlohmann/json.hpp>

namespace gradium {
namespace {

using json = nlohmann::json;

PronunciationDictionary parseDictJson(const json& payload)
{
    PronunciationDictionary dict;
    dict.uid        = detail::stringOrEmpty(payload, "uid");
    dict.org_uid    = detail::stringOrEmpty(payload, "org_uid");
    dict.created_at = detail::stringOrEmpty(payload, "created_at");

    if (payload.contains("rules") && payload["rules"].is_array()) {
        for (const auto& r : payload["rules"]) {
            PronunciationRule rule;
            rule.id             = detail::stringOrEmpty(r, "id");
            rule.original       = detail::stringOrEmpty(r, "original");
            rule.rewrite        = detail::stringOrEmpty(r, "rewrite");
            rule.case_sensitive = r.value("case_sensitive", false);
            dict.rules.push_back(std::move(rule));
        }
    }
    return dict;
}

json serializeRules(const std::vector<PronunciationRule>& rules)
{
    json arr = json::array();
    for (const auto& rule : rules) {
        json r;
        r["original"]       = rule.original;
        r["rewrite"]        = rule.rewrite;
        r["case_sensitive"] = rule.case_sensitive;
        arr.push_back(std::move(r));
    }
    return arr;
}

} // namespace

PronunciationClient::PronunciationClient(
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

transport::HttpRequest PronunciationClient::makeRequest(
    transport::HttpMethod method,
    const std::string& path,
    const std::string& json_body) const
{
    transport::HttpRequest req;
    req.method = method;
    req.url    = _baseUrl + path;
    req.headers["x-api-key"]  = _apiKey;
    req.headers["User-Agent"] = gradium::USER_AGENT;
    if (!json_body.empty()) {
        req.content_type = "application/json";
        req.body = json_body;
    }
    return req;
}

PronunciationDictionary PronunciationClient::parseDictionary(const std::string& body) const
{
    return parseDictJson(json::parse(body));
}

std::string PronunciationClient::listDictionaries()
{
    auto req = makeRequest(transport::HttpMethod::Get, pronunciations::endpoints::base);
    auto response = _httpTransport->send(req);
    detail::throwIfError(response, "listDictionaries");
    return std::string(response.body.begin(), response.body.end());
}

std::vector<PronunciationDictionary> PronunciationClient::listDictionariesTyped()
{
    const std::string body = listDictionaries();
    const auto payload = json::parse(body, nullptr, false);
    std::vector<PronunciationDictionary> dicts;
    if (payload.is_discarded()) return dicts;

    const auto& arr = payload.is_array() ? payload : payload.value("items", json::array());
    for (const auto& d : arr) {
        dicts.push_back(parseDictJson(d));
    }
    return dicts;
}

std::string PronunciationClient::createDictionary(const PronunciationCreateRequest& request)
{
    json body;
    body["name"]     = request.name;
    body["language"] = request.language;
    if (!request.description.empty()) body["description"] = request.description;
    if (!request.rules.empty())       body["rules"]       = serializeRules(request.rules);

    auto req = makeRequest(transport::HttpMethod::Post, pronunciations::endpoints::base, body.dump());
    auto response = _httpTransport->send(req);
    detail::throwIfError(response, "createDictionary");
    return std::string(response.body.begin(), response.body.end());
}

PronunciationDictionary PronunciationClient::createDictionaryTyped(
    const PronunciationCreateRequest& request)
{
    return parseDictionary(createDictionary(request));
}

std::string PronunciationClient::getDictionary(const std::string& uid)
{
    auto req = makeRequest(transport::HttpMethod::Get, pronunciations::endpoints::base + uid);
    auto response = _httpTransport->send(req);
    detail::throwIfError(response, "getDictionary");
    return std::string(response.body.begin(), response.body.end());
}

PronunciationDictionary PronunciationClient::getDictionaryTyped(const std::string& uid)
{
    return parseDictionary(getDictionary(uid));
}

std::string PronunciationClient::updateDictionary(const std::string& uid,
                                                   const PronunciationUpdateRequest& request)
{
    json body;
    if (!request.name.empty())        body["name"]        = request.name;
    if (!request.description.empty()) body["description"] = request.description;
    if (!request.rules.empty())       body["rules"]       = serializeRules(request.rules);

    auto req = makeRequest(transport::HttpMethod::Put, pronunciations::endpoints::base + uid, body.dump());
    auto response = _httpTransport->send(req);
    detail::throwIfError(response, "updateDictionary");
    return std::string(response.body.begin(), response.body.end());
}

PronunciationDictionary PronunciationClient::updateDictionaryTyped(
    const std::string& uid,
    const PronunciationUpdateRequest& request)
{
    return parseDictionary(updateDictionary(uid, request));
}

void PronunciationClient::deleteDictionary(const std::string& uid)
{
    auto req = makeRequest(transport::HttpMethod::Delete, pronunciations::endpoints::base + uid);
    auto response = _httpTransport->send(req);
    detail::throwIfError(response, "deleteDictionary");
}

} // namespace gradium
