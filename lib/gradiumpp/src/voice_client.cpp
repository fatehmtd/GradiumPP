#include <gradiumpp/voice_client.hpp>
#include <gradiumpp/transport/curl_http_transport.hpp>

#include "error_util.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>

namespace gradium {
namespace {

using json = nlohmann::json;

Voice parseVoiceJson(const json& v)
{
    Voice voice;
    voice.uid          = detail::stringOrEmpty(v, "uid");
    voice.name         = detail::stringOrEmpty(v, "name");
    voice.description  = detail::stringOrEmpty(v, "description");
    voice.filename     = detail::stringOrEmpty(v, "filename");
    voice.language     = detail::stringOrEmpty(v, "language");
    voice.org_uid      = detail::stringOrEmpty(v, "org_uid");
    voice.start_s      = v.value("start_s", 0.0);
    voice.stop_s       = v.value("stop_s", 0.0);
    voice.is_catalog   = v.value("is_catalog", false);
    voice.is_pro_clone = v.value("is_pro_clone", false);
    voice.is_pending   = v.value("is_pending", false);
    voice.has_audio    = v.value("has_audio", true);

    if (v.contains("tags") && v["tags"].is_array()) {
        for (const auto& tag : v["tags"]) {
            if (tag.is_object()) {
                VoiceTag vt;
                vt.category = detail::stringOrEmpty(tag, "category");
                vt.value    = detail::stringOrEmpty(tag, "value");
                voice.tags.push_back(std::move(vt));
            } else if (tag.is_string()) {
                voice.tags.push_back(VoiceTag{"", tag.get<std::string>()});
            }
        }
    }
    return voice;
}

json serializeTags(const std::vector<VoiceTag>& tags)
{
    json arr = json::array();
    for (const auto& tag : tags) {
        json t;
        if (!tag.category.empty()) t["category"] = tag.category;
        t["value"] = tag.value;
        arr.push_back(std::move(t));
    }
    return arr;
}

} // namespace

VoiceClient::VoiceClient(
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

transport::HttpRequest VoiceClient::makeRequest(
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

Voice VoiceClient::parseVoice(const std::string& body_json) const
{
    return parseVoiceJson(json::parse(body_json));
}

std::string VoiceClient::listVoices(bool include_catalog)
{
    std::string url = _baseUrl + voices::endpoints::base;
    if (include_catalog) url += "?include_catalog=true";
    auto req = makeRequest(transport::HttpMethod::Get, "");
    req.url  = url;
    auto response = _httpTransport->send(req);
    detail::throwIfError(response, "listVoices");
    return std::string(response.body.begin(), response.body.end());
}

std::vector<Voice> VoiceClient::listVoicesTyped(bool include_catalog)
{
    const std::string body = listVoices(include_catalog);
    const auto payload = json::parse(body, nullptr, false);
    std::vector<Voice> voices;
    if (payload.is_discarded()) return voices;

    const auto& arr = payload.is_array() ? payload : payload.value("voices", json::array());
    for (const auto& v : arr) {
        voices.push_back(parseVoiceJson(v));
    }
    return voices;
}

std::string VoiceClient::createVoice(const VoiceCreateRequest& request)
{
    transport::HttpRequest req;
    req.method = transport::HttpMethod::Post;
    req.url    = _baseUrl + voices::endpoints::base;
    req.headers["x-api-key"]  = _apiKey;
    req.headers["User-Agent"] = gradium::USER_AGENT;

    transport::MultipartFile file;
    file.field_name   = "audio_file";
    file.file_path    = request.audio_file_path;
    req.multipart_files.push_back(std::move(file));

    req.multipart_fields.push_back({"name", request.name, {}});
    if (!request.description.empty()) {
        req.multipart_fields.push_back({"description", request.description, {}});
    }
    if (!request.language.empty()) {
        req.multipart_fields.push_back({"language", request.language, {}});
    }
    if (!request.input_format.empty()) {
        req.multipart_fields.push_back({"input_format", request.input_format, {}});
    }
    if (request.start_s != 0.0) {
        req.multipart_fields.push_back({"start_s", std::to_string(request.start_s), {}});
    }
    if (request.timeout_s != 10.0) {
        req.multipart_fields.push_back({"timeout_s", std::to_string(request.timeout_s), {}});
    }

    auto response = _httpTransport->send(req);
    detail::throwIfError(response, "createVoice");
    return std::string(response.body.begin(), response.body.end());
}

Voice VoiceClient::createVoiceTyped(const VoiceCreateRequest& request)
{
    // POST /voices/ only returns {uid, error, was_updated} — fetch the full
    // voice afterward so callers get back a complete Voice, same as getVoiceTyped().
    const auto ack = json::parse(createVoice(request), nullptr, false);
    if (ack.is_discarded()) {
        throw std::runtime_error("[gradiumpp] createVoice: malformed response body");
    }

    const std::string error = detail::stringOrEmpty(ack, "error");
    if (!error.empty()) {
        throw std::runtime_error("[gradiumpp] createVoice failed: " + error);
    }

    const std::string uid = detail::stringOrEmpty(ack, "uid");
    return getVoiceTyped(uid);
}

std::string VoiceClient::getVoice(const std::string& uid)
{
    auto req = makeRequest(transport::HttpMethod::Get, voices::endpoints::base + uid);
    auto response = _httpTransport->send(req);
    detail::throwIfError(response, "getVoice");
    return std::string(response.body.begin(), response.body.end());
}

Voice VoiceClient::getVoiceTyped(const std::string& uid)
{
    return parseVoice(getVoice(uid));
}

std::string VoiceClient::updateVoice(const std::string& uid,
                                      const VoiceUpdateRequest& request)
{
    json body;
    if (!request.name.empty())        body["name"]        = request.name;
    if (!request.description.empty()) body["description"] = request.description;
    if (!request.language.empty())    body["language"]    = request.language;
    if (!request.tags.empty())        body["tags"]        = serializeTags(request.tags);
    if (request.start_s.has_value())  body["start_s"]      = *request.start_s;
    if (request.rank.has_value())     body["rank"]         = *request.rank;

    auto req = makeRequest(transport::HttpMethod::Put, voices::endpoints::base + uid, body.dump());
    auto response = _httpTransport->send(req);
    detail::throwIfError(response, "updateVoice");
    return std::string(response.body.begin(), response.body.end());
}

Voice VoiceClient::updateVoiceTyped(const std::string& uid,
                                     const VoiceUpdateRequest& request)
{
    return parseVoice(updateVoice(uid, request));
}

void VoiceClient::deleteVoice(const std::string& uid)
{
    auto req = makeRequest(transport::HttpMethod::Delete, voices::endpoints::base + uid);
    auto response = _httpTransport->send(req);
    detail::throwIfError(response, "deleteVoice");
}

} // namespace gradium
