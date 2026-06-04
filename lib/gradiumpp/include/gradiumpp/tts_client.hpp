#pragma once

#include <gradiumpp/types.hpp>
#include <gradiumpp/transport/http_transport.hpp>
#include <gradiumpp/transport/websocket_transport.hpp>

#include <functional>
#include <memory>
#include <string>

namespace gradium {

// ---------------------------------------------------------------------------
// REST TTS  (POST /post/speech/tts)
// ---------------------------------------------------------------------------

/// REST client for Gradium Text-to-Speech.
class TtsRestClient {
public:
    explicit TtsRestClient(
        std::string api_key,
        std::shared_ptr<transport::IHttpTransport> http_transport = nullptr,
        std::string base_url = gradium::api::base_url);

    /* Returns HttpResponse whose body contains raw audio bytes when config.only_audio=true,
       or streaming NDJSON with base64 audio chunks when only_audio=false.
       Throws GradiumApiException on HTTP >= 400. */
    transport::HttpResponse generateSpeech(const TtsConfig& config, const std::string& text);

private:
    std::string _apiKey;
    std::string _baseUrl;
    std::shared_ptr<transport::IHttpTransport> _httpTransport;
};

// ---------------------------------------------------------------------------
// Real-time TTS WebSocket  (wss://api.gradium.ai/api/speech/tts)
// ---------------------------------------------------------------------------

/* Setup parameters for one TTS stream (or one multiplexed request).
   Call setup() after connect(); wait for "ready" before sending text. */
struct TtsRealtimeSetup {
    std::string voice_id;
    std::string model_name{tts::models::default_model};
    std::string output_format{tts::output_formats::wav};
    std::string client_req_id;       ///< optional; required for multiplexing
    bool        close_ws_on_eos{true}; ///< set false to keep connection open for next request
};

/* Event-driven WebSocket client for Gradium real-time TTS.
   Protocol: connect → setup → (wait for "ready") → sendText → (receive "audio" chunks) → sendEndOfStream */
class TtsRealtimeClient {
public:
    using TextMessageCallback   = std::function<void(const std::string&)>;
    using ParsedMessageCallback = std::function<void(const TtsRealtimeMessage&)>;
    using ErrorCallback         = std::function<void(const std::string&)>;
    using ClosedCallback        = std::function<void()>;

    explicit TtsRealtimeClient(
        std::shared_ptr<transport::IWebSocketTransport> ws_transport = nullptr,
        std::string endpoint = gradium::tts::endpoints::realtime);

    void setOnMessage(TextMessageCallback callback);
    void setOnParsedMessage(ParsedMessageCallback callback);
    void setOnError(ErrorCallback callback);
    void setOnClosed(ClosedCallback callback);

    /* Opens the WebSocket connection with x-api-key authentication.
       Blocks until the handshake completes. Throws std::runtime_error on failure. */
    void connect(const std::string& api_key);

    /// Sends {"type":"setup",...}. Server responds with {"type":"ready",...}.
    void setup(const TtsRealtimeSetup& cfg);

    /// Sends {"type":"text","text":"..."}. Call after receiving "ready".
    void sendText(const std::string& text, const std::string& client_req_id = "") const;

    /// Sends {"type":"end_of_stream"}. Server echoes it back then closes (if close_ws_on_eos).
    void sendEndOfStream(const std::string& client_req_id = "") const;

    void close();

    TtsRealtimeMessage parseMessage(const std::string& raw) const;

private:
    std::string _endpoint;
    std::shared_ptr<transport::IWebSocketTransport> _wsTransport;
    TextMessageCallback   _onMessage;
    ParsedMessageCallback _onParsedMessage;
    ErrorCallback         _onError;
    ClosedCallback        _onClosed;
};

} // namespace gradium
