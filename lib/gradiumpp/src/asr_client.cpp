#include <gradiumpp/asr_client.hpp>
#include <gradiumpp/transport/curl_http_transport.hpp>
#ifdef _WIN32
#include <gradiumpp/transport/winhttp_websocket_transport.hpp>
#else
#include <gradiumpp/transport/lws_websocket_transport.hpp>
#endif

#include "error_util.hpp"

#include <cppcodec/base64_rfc4648.hpp>

#include <nlohmann/json.hpp>

#include <cctype>
#include <fstream>
#include <stdexcept>

namespace gradium {
namespace {

using json = nlohmann::json;

std::string audioContentType(const std::string& input_format)
{
    if (input_format == "wav")  return "audio/wav";
    if (input_format == "opus") return "audio/opus";
    return "audio/pcm";
}

std::string urlEncode(const std::string& value)
{
    static const char hex[] = "0123456789ABCDEF";
    std::string result;
    result.reserve(value.size() * 3);
    for (unsigned char c : value) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            result += static_cast<char>(c);
        } else {
            result += '%';
            result += hex[(c >> 4) & 0xF];
            result += hex[c & 0xF];
        }
    }
    return result;
}

AsrRealtimeMessage parseAsrMessage(const std::string& raw)
{
    AsrRealtimeMessage msg;
    msg.raw_message = raw;

    const json payload = json::parse(raw, nullptr, false);
    if (payload.is_discarded()) {
        return msg;
    }

    msg.type          = detail::stringOrEmpty(payload, "type");
    msg.session_id    = detail::stringOrEmpty(payload, "session_id");
    msg.flush_id      = detail::stringOrEmpty(payload, "flush_id");
    msg.client_req_id = detail::stringOrEmpty(payload, "client_req_id");
    msg.error_message = detail::stringOrEmpty(payload, "message");

    if (payload.contains("sample_rate") && payload["sample_rate"].is_number())
        msg.sample_rate = payload["sample_rate"].get<int>();
    if (payload.contains("frame_size") && payload["frame_size"].is_number())
        msg.frame_size = payload["frame_size"].get<int>();

    if (payload.contains("vad") && payload["vad"].is_array()) {
        for (const auto& item : payload["vad"]) {
            if (!item.is_object()) continue;
            const double horizon = item.value("horizon_s",      0.0);
            const double prob    = item.value("inactivity_prob", 0.0);
            if      (horizon == 0.5) msg.vad.inactivity_prob_0_5s = prob;
            else if (horizon == 1.0) msg.vad.inactivity_prob_1_0s = prob;
            else if (horizon == 2.0) msg.vad.inactivity_prob_2_0s = prob;
        }
    }

    msg.segment.text      = detail::stringOrEmpty(payload, "text");
    msg.segment.stream_id = detail::stringOrEmpty(payload, "stream_id");
    if (payload.contains("start_s") && payload["start_s"].is_number())
        msg.segment.start_s = payload["start_s"].get<double>();
    if (payload.contains("stop_s") && payload["stop_s"].is_number())
        msg.segment.stop_s = payload["stop_s"].get<double>();

    return msg;
}

} // namespace

// ---------------------------------------------------------------------------
// AsrRestClient
// ---------------------------------------------------------------------------

AsrRestClient::AsrRestClient(
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

std::string AsrRestClient::buildQueryString(const AsrConfig& config) const
{
    std::string qs = "?input_format=" + urlEncode(config.input_format)
                   + "&model_name="   + urlEncode(config.model_name);

    json jconfig;
    if (config.json_config.temp != 0.0)
        jconfig["temp"] = config.json_config.temp;
    if (!config.json_config.language.empty())
        jconfig["language"] = config.json_config.language;
    if (config.json_config.padding_bonus != 0.0)
        jconfig["padding_bonus"] = config.json_config.padding_bonus;
    if (config.json_config.delay_in_frames != 0)
        jconfig["delay_in_frames"] = config.json_config.delay_in_frames;

    if (!jconfig.empty()) {
        qs += "&json_config=" + urlEncode(jconfig.dump());
    }
    return qs;
}

std::string AsrRestClient::transcribe(const AsrConfig& config,
                                       const std::vector<std::uint8_t>& audio_bytes)
{
    transport::HttpRequest req;
    req.method       = transport::HttpMethod::Post;
    req.url          = _baseUrl + asr::endpoints::rest + buildQueryString(config);
    req.content_type = audioContentType(config.input_format);
    req.binary_body  = audio_bytes;
    req.headers["x-api-key"]  = _apiKey;
    req.headers["User-Agent"] = gradium::USER_AGENT;

    auto response = _httpTransport->send(req);
    detail::throwIfError(response, "ASR transcribe");

    return std::string(response.body.begin(), response.body.end());
}

std::string AsrRestClient::transcribeFile(const std::string& file_path,
                                           const AsrConfig& config)
{
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("[gradiumpp] Cannot open audio file: " + file_path);
    }
    const std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    return transcribe(config, bytes);
}

// ---------------------------------------------------------------------------
// AsrRealtimeClient
// ---------------------------------------------------------------------------

AsrRealtimeClient::AsrRealtimeClient(
    std::shared_ptr<transport::IWebSocketTransport> ws_transport,
    std::string endpoint)
    : _endpoint(std::move(endpoint)),
      _wsTransport(std::move(ws_transport))
{
    if (!_wsTransport) {
#ifdef _WIN32
        _wsTransport = std::make_shared<transport::WinHttpWebSocketTransport>();
#else
        _wsTransport = std::make_shared<transport::LwsWebSocketTransport>();
#endif
    }

    _wsTransport->setOnTextMessage([this](const std::string& message) {
        if (_onMessage) {
            _onMessage(message);
        }
        if (_onParsedMessage) {
            _onParsedMessage(parseMessage(message));
        }
    });

    _wsTransport->setOnError([this](const std::string& error) {
        if (_onError) {
            _onError(error);
        }
    });

    _wsTransport->setOnClose([this] {
        if (_onClosed) {
            _onClosed();
        }
    });
}

void AsrRealtimeClient::setOnMessage(TextMessageCallback callback)   { _onMessage = std::move(callback); }
void AsrRealtimeClient::setOnParsedMessage(ParsedMessageCallback cb) { _onParsedMessage = std::move(cb); }
void AsrRealtimeClient::setOnError(ErrorCallback callback)           { _onError = std::move(callback); }
void AsrRealtimeClient::setOnClosed(ClosedCallback callback)         { _onClosed = std::move(callback); }

void AsrRealtimeClient::connect(const std::string& api_key,
                                 const AsrRealtimeSetup& setup)
{
    transport::WebSocketConnectOptions options;
    options.url = _endpoint;
    options.headers["x-api-key"]  = api_key;
    options.headers["User-Agent"] = gradium::USER_AGENT;
    _wsTransport->connect(options);

    json payload;
    payload["type"]         = "setup";
    payload["input_format"] = setup.input_format;
    payload["model_name"]   = setup.model_name;
    if (!setup.client_req_id.empty())
        payload["client_req_id"] = setup.client_req_id;
    if (!setup.close_ws_on_eos)
        payload["close_ws_on_eos"] = false;

    _wsTransport->sendText(payload.dump());
}

void AsrRealtimeClient::sendAudio(const std::vector<std::uint8_t>& pcm_chunk)
{
    const std::string encoded = cppcodec::base64_rfc4648::encode(pcm_chunk.data(), pcm_chunk.size());
    json payload;
    payload["type"]  = "audio";
    payload["audio"] = encoded;
    _wsTransport->sendText(payload.dump());
}

void AsrRealtimeClient::sendFlush(const std::string& flush_id)
{
    json payload;
    payload["type"]     = "flush";
    payload["flush_id"] = flush_id;
    _wsTransport->sendText(payload.dump());
}

void AsrRealtimeClient::sendEndOfStream()
{
    json payload;
    payload["type"] = "end_of_stream";
    _wsTransport->sendText(payload.dump());
}

void AsrRealtimeClient::close()
{
    _wsTransport->close();
}

AsrRealtimeMessage AsrRealtimeClient::parseMessage(const std::string& raw) const
{
    return parseAsrMessage(raw);
}

} // namespace gradium
