#include <gradiumpp/tts_client.hpp>
#include <gradiumpp/transport/curl_http_transport.hpp>
#ifdef _WIN32
#include <gradiumpp/transport/winhttp_websocket_transport.hpp>
#else
#include <gradiumpp/transport/lws_websocket_transport.hpp>
#endif

#include "error_util.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>

namespace gradium {
namespace {

using json = nlohmann::json;

TtsRealtimeMessage parseTtsMessage(const std::string& raw)
{
    TtsRealtimeMessage msg;
    msg.raw_message = raw;

    const json payload = json::parse(raw, nullptr, false);
    if (payload.is_discarded()) {
        return msg;
    }

    msg.type           = detail::stringOrEmpty(payload, "type");
    msg.session_id     = detail::stringOrEmpty(payload, "session_id");
    msg.audio_base64   = detail::stringOrEmpty(payload, "audio");
    msg.text           = detail::stringOrEmpty(payload, "text");
    msg.stream_id      = detail::stringOrEmpty(payload, "stream_id");
    msg.client_req_id  = detail::stringOrEmpty(payload, "client_req_id");
    msg.error_message  = detail::stringOrEmpty(payload, "message");
    msg.start_s        = payload.value("start_s", 0.0);

    return msg;
}

} // namespace

// ---------------------------------------------------------------------------
// TtsRestClient
// ---------------------------------------------------------------------------

TtsRestClient::TtsRestClient(
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

transport::HttpResponse TtsRestClient::generateSpeech(const TtsConfig& config,
                                                        const std::string& text)
{
    json body;
    body["text"]          = text;
    body["voice_id"]      = config.voice_id;
    body["output_format"] = config.output_format;
    body["only_audio"]    = config.only_audio;
    body["model_name"]    = config.model_name;

    json jconfig;
    if (config.json_config.temp != 0.0)          jconfig["temp"]          = config.json_config.temp;
    if (config.json_config.cfg_coef != 0.0)      jconfig["cfg_coef"]      = config.json_config.cfg_coef;
    if (config.json_config.padding_bonus != 0.0) jconfig["padding_bonus"] = config.json_config.padding_bonus;
    if (!config.json_config.pronunciation_id.empty())
        jconfig["pronunciation_id"] = config.json_config.pronunciation_id;
    if (!config.json_config.rewrite_rules.empty())
        jconfig["rewrite_rules"] = config.json_config.rewrite_rules;
    if (!jconfig.is_null() && !jconfig.empty())
        body["json_config"] = jconfig;

    transport::HttpRequest req;
    req.method       = transport::HttpMethod::Post;
    req.url          = _baseUrl + tts::endpoints::rest;
    req.content_type = "application/json";
    req.body         = body.dump();
    req.headers["x-api-key"]   = _apiKey;
    req.headers["User-Agent"]  = gradium::USER_AGENT;

    auto response = _httpTransport->send(req);
    detail::throwIfError(response, "TTS generateSpeech");
    return response;
}

// ---------------------------------------------------------------------------
// TtsRealtimeClient
// ---------------------------------------------------------------------------

TtsRealtimeClient::TtsRealtimeClient(
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

void TtsRealtimeClient::setOnMessage(TextMessageCallback callback)
{
    _onMessage = std::move(callback);
}

void TtsRealtimeClient::setOnParsedMessage(ParsedMessageCallback callback)
{
    _onParsedMessage = std::move(callback);
}

void TtsRealtimeClient::setOnError(ErrorCallback callback)
{
    _onError = std::move(callback);
}

void TtsRealtimeClient::setOnClosed(ClosedCallback callback)
{
    _onClosed = std::move(callback);
}

void TtsRealtimeClient::connect(const std::string& api_key)
{
    transport::WebSocketConnectOptions options;
    options.url = _endpoint;
    options.headers["x-api-key"]  = api_key;
    options.headers["User-Agent"] = gradium::USER_AGENT;
    _wsTransport->connect(options);
}

void TtsRealtimeClient::setup(const TtsRealtimeSetup& cfg)
{
    json payload;
    payload["type"]          = "setup";
    payload["voice_id"]      = cfg.voice_id;
    payload["model_name"]    = cfg.model_name;
    payload["output_format"] = cfg.output_format;
    if (!cfg.client_req_id.empty())
        payload["client_req_id"] = cfg.client_req_id;
    if (!cfg.close_ws_on_eos)
        payload["close_ws_on_eos"] = false;

    _wsTransport->sendText(payload.dump());
}

void TtsRealtimeClient::sendText(const std::string& text,
                                  const std::string& client_req_id)
{
    json payload;
    payload["type"] = "text";
    payload["text"] = text;
    if (!client_req_id.empty())
        payload["client_req_id"] = client_req_id;
    _wsTransport->sendText(payload.dump());
}

void TtsRealtimeClient::sendEndOfStream(const std::string& client_req_id)
{
    json payload;
    payload["type"] = "end_of_stream";
    if (!client_req_id.empty())
        payload["client_req_id"] = client_req_id;
    _wsTransport->sendText(payload.dump());
}

void TtsRealtimeClient::close()
{
    _wsTransport->close();
}

TtsRealtimeMessage TtsRealtimeClient::parseMessage(const std::string& raw) const
{
    return parseTtsMessage(raw);
}

} // namespace gradium
