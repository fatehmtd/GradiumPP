#include <gradiumpp/s2s_client.hpp>
#include <gradiumpp/transport/curl_http_transport.hpp>
#ifdef _WIN32
#include <gradiumpp/transport/winhttp_websocket_transport.hpp>
#else
#include <gradiumpp/transport/lws_websocket_transport.hpp>
#endif

#include "error_util.hpp"

#include <cppcodec/base64_rfc4648.hpp>

#include <nlohmann/json.hpp>

namespace gradium {
namespace {

using json = nlohmann::json;

S2sRealtimeMessage parseS2sMessage(const std::string& raw)
{
    S2sRealtimeMessage msg;
    msg.raw_message = raw;

    const json payload = json::parse(raw, nullptr, false);
    if (payload.is_discarded()) {
        return msg;
    }

    msg.type_str = detail::stringOrEmpty(payload, "type");

    if (msg.type_str == "ready") msg.type = S2sRealtimeMessage::Type::Ready;
    else if (msg.type_str == "audio") msg.type = S2sRealtimeMessage::Type::Audio;
    else if (msg.type_str == "text") msg.type = S2sRealtimeMessage::Type::Text;
    else if (msg.type_str == "end_of_stream") msg.type = S2sRealtimeMessage::Type::EndOfStream;
    else if (msg.type_str == "error") msg.type = S2sRealtimeMessage::Type::Error;
    else msg.type = S2sRealtimeMessage::Type::UNKNOWN;

    msg.request_id    = detail::stringOrEmpty(payload, "request_id");
    msg.audio_base64  = detail::stringOrEmpty(payload, "audio");
    msg.text          = detail::stringOrEmpty(payload, "text");
    msg.stream_id     = detail::stringOrEmpty(payload, "stream_id");
    msg.client_req_id = detail::stringOrEmpty(payload, "client_req_id");
    msg.error_message = detail::stringOrEmpty(payload, "message");
    msg.start_s       = payload.value("start_s", 0.0);
    msg.stop_s        = payload.value("stop_s", 0.0);
    msg.sample_rate   = payload.value("sample_rate", 0);
    msg.frame_size    = payload.value("frame_size", 0);
    msg.error_code    = payload.value("code", 0);

    return msg;
}

} // namespace

S2sRealtimeClient::S2sRealtimeClient(
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

void S2sRealtimeClient::setOnMessage(TextMessageCallback callback)
{
    _onMessage = std::move(callback);
}

void S2sRealtimeClient::setOnParsedMessage(ParsedMessageCallback callback)
{
    _onParsedMessage = std::move(callback);
}

void S2sRealtimeClient::setOnError(ErrorCallback callback)
{
    _onError = std::move(callback);
}

void S2sRealtimeClient::setOnClosed(ClosedCallback callback)
{
    _onClosed = std::move(callback);
}

void S2sRealtimeClient::connect(const std::string& api_key, const S2sRealtimeSetup& setup)
{
    transport::WebSocketConnectOptions options;
    options.url = _endpoint;
    options.headers["x-api-key"]  = api_key;
    options.headers["User-Agent"] = gradium::USER_AGENT;
    _wsTransport->connect(options);

    json payload;
    payload["type"]          = "setup";
    payload["model_name"]    = setup.model_name;
    payload["input_format"]  = setup.input_format;
    payload["output_format"] = setup.output_format;
    if (!setup.stt_model_name.empty()) payload["stt_model_name"] = setup.stt_model_name;
    if (!setup.tts_model_name.empty()) payload["tts_model_name"] = setup.tts_model_name;
    if (!setup.voice_id.empty())       payload["voice_id"]       = setup.voice_id;
    if (!setup.client_req_id.empty())  payload["client_req_id"]  = setup.client_req_id;
    if (!setup.close_ws_on_eos)        payload["close_ws_on_eos"] = false;
    if (setup.retry_for_s != 0.0)      payload["retry_for_s"]    = setup.retry_for_s;
    if (!setup.target_language.empty()) {
        payload["json_config"] = json{{"target_language", setup.target_language}};
    }

    _wsTransport->sendText(payload.dump());
}

void S2sRealtimeClient::sendAudio(const std::vector<std::uint8_t>& pcm_chunk,
                                   const std::string& client_req_id) const
{
    sendAudio(pcm_chunk.data(), pcm_chunk.size(), client_req_id);
}

void S2sRealtimeClient::sendAudio(const std::uint8_t* pcm_samples, std::size_t sample_count,
                                   const std::string& client_req_id) const
{
    const std::string encoded = cppcodec::base64_rfc4648::encode(pcm_samples, sample_count);
    json payload;
    payload["type"]  = "audio";
    payload["audio"] = encoded;
    if (!client_req_id.empty())
        payload["client_req_id"] = client_req_id;
    _wsTransport->sendText(payload.dump());
}

void S2sRealtimeClient::sendEndOfStream(const std::string& client_req_id) const
{
    json payload;
    payload["type"] = "end_of_stream";
    if (!client_req_id.empty())
        payload["client_req_id"] = client_req_id;
    _wsTransport->sendText(payload.dump());
}

void S2sRealtimeClient::close()
{
    _wsTransport->close();
}

S2sRealtimeMessage S2sRealtimeClient::parseMessage(const std::string& raw) const
{
    return parseS2sMessage(raw);
}

} // namespace gradium
