#pragma once

#include <gradiumpp/types.hpp>
#include <gradiumpp/transport/websocket_transport.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace gradium {

// ---------------------------------------------------------------------------
// Real-time S2S WebSocket  (wss://api.gradium.ai/api/speech/s2s)
// ---------------------------------------------------------------------------

/* Setup parameters for one S2S stream. Incoming audio is transcribed,
   translated into target_language, and re-synthesized as speech. */
struct S2sRealtimeSetup {
    std::string input_format{asr::input_formats::wav};
    std::string output_format{tts::output_formats::wav};
    std::string model_name{s2s::models::default_model};
    std::string stt_model_name{s2s::models::default_stt_model};
    std::string tts_model_name{s2s::models::default_tts_model};
    std::string voice_id;             ///< voice used for the synthesized output
    std::string target_language;      ///< required by "s2s-translate" (json_config.target_language)
    std::string client_req_id;        ///< optional; required for multiplexing
    bool        close_ws_on_eos{true};
    double      retry_for_s{0.0};     ///< optional setup retry window for transient worker allocation failures; 0 = unset
};

/* Event-driven WebSocket client for Gradium real-time Speech-to-Speech.
   Protocol: connect (sends setup) → (wait for "ready") → sendAudio loop → sendEndOfStream
   Server streams back "text" (transcript, optionally translated) and "audio"
   (re-synthesized speech) messages interleaved. */
class S2sRealtimeClient {
public:
    using TextMessageCallback   = std::function<void(const std::string&)>;
    using ParsedMessageCallback = std::function<void(const S2sRealtimeMessage&)>;
    using ErrorCallback         = std::function<void(const std::string&)>;
    using ClosedCallback        = std::function<void()>;

    explicit S2sRealtimeClient(
        std::shared_ptr<transport::IWebSocketTransport> ws_transport = nullptr,
        std::string endpoint = gradium::s2s::endpoints::realtime);

    void setOnMessage(TextMessageCallback callback);
    void setOnParsedMessage(ParsedMessageCallback callback);
    void setOnError(ErrorCallback callback);
    void setOnClosed(ClosedCallback callback);

    /* Opens the WebSocket with x-api-key authentication and sends the setup message.
       Blocks until the handshake completes. Throws std::runtime_error on failure. */
    void connect(const std::string& api_key, const S2sRealtimeSetup& setup);

    /* Send a raw PCM audio chunk. The library encodes it to base64 internally.
       Recommended chunk size: 1920 samples × 2 bytes = 3840 bytes (80 ms at 24 kHz). */
    void sendAudio(const std::vector<std::uint8_t>& pcm_chunk, const std::string& client_req_id = "") const;

    void sendAudio(const std::uint8_t* pcm_samples, std::size_t sample_count,
                   const std::string& client_req_id = "") const;

    /// Sends {"type":"end_of_stream"}.
    void sendEndOfStream(const std::string& client_req_id = "") const;

    void close();

    S2sRealtimeMessage parseMessage(const std::string& raw) const;

private:
    std::string _endpoint;
    std::shared_ptr<transport::IWebSocketTransport> _wsTransport;
    TextMessageCallback   _onMessage;
    ParsedMessageCallback _onParsedMessage;
    ErrorCallback         _onError;
    ClosedCallback        _onClosed;
};

} // namespace gradium
