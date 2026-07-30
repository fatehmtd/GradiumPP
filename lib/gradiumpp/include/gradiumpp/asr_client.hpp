#pragma once

#include <gradiumpp/types.hpp>
#include <gradiumpp/transport/http_transport.hpp>
#include <gradiumpp/transport/websocket_transport.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace gradium {

// ---------------------------------------------------------------------------
// REST ASR  (POST /post/speech/asr)
// ---------------------------------------------------------------------------

/// REST client for Gradium Automatic Speech Recognition.
class AsrRestClient {
public:
    explicit AsrRestClient(
        std::string api_key,
        std::shared_ptr<transport::IHttpTransport> http_transport = nullptr,
        std::string base_url = gradium::api::base_url);

    /* Sends raw audio bytes as the POST body; config fields become URL query parameters.
       Returns the NDJSON response body as a string.
       Throws GradiumApiException on HTTP >= 400. */
    std::string transcribe(const AsrConfig& config,
                           const std::vector<std::uint8_t>& audio_bytes);

    /// Convenience: reads the file at file_path and calls transcribe().
    std::string transcribeFile(const std::string& file_path, const AsrConfig& config);

private:
    std::string _apiKey;
    std::string _baseUrl;
    std::shared_ptr<transport::IHttpTransport> _httpTransport;

    std::string buildQueryString(const AsrConfig& config) const;
};

// ---------------------------------------------------------------------------
// Real-time ASR WebSocket  (wss://api.gradium.ai/api/speech/asr)
// ---------------------------------------------------------------------------

/* Setup parameters for one ASR stream. */
struct AsrRealtimeSetup {
    std::string input_format{asr::input_formats::pcm};
    std::string model_name{asr::models::default_model};
    std::string client_req_id;        ///< optional; required for multiplexing
    bool        close_ws_on_eos{true};
    double      retry_for_s{0.0};     ///< optional setup retry window for transient worker allocation failures; 0 = unset
    AsrJsonConfig json_config;        ///< advanced settings: temp, language, target_language, padding_bonus, delay_in_frames, keywords
};

/* Event-driven WebSocket client for Gradium real-time ASR.
   Protocol: connect → (receives "ready") → sendAudio loop → sendFlush → sendEndOfStream
   VAD "step" messages arrive every 80 ms during audio processing. */
class AsrRealtimeClient {
public:
    using TextMessageCallback   = std::function<void(const std::string&)>;
    using ParsedMessageCallback = std::function<void(const AsrRealtimeMessage&)>;
    using ErrorCallback         = std::function<void(const std::string&)>;
    using ClosedCallback        = std::function<void()>;

    explicit AsrRealtimeClient(
        std::shared_ptr<transport::IWebSocketTransport> ws_transport = nullptr,
        std::string endpoint = gradium::asr::endpoints::realtime);

    void setOnMessage(TextMessageCallback callback);
    void setOnParsedMessage(ParsedMessageCallback callback);
    void setOnError(ErrorCallback callback);
    void setOnClosed(ClosedCallback callback);

    /* Opens the WebSocket with x-api-key authentication and sends the setup message.
       Blocks until the handshake completes. Throws std::runtime_error on failure. */
    void connect(const std::string& api_key, const AsrRealtimeSetup& setup);

    /* Send a raw PCM audio chunk. The library encodes it to base64 internally.
       Recommended chunk size: 1920 samples × 2 bytes = 3840 bytes (80 ms at 24 kHz). */
    void sendAudio(const std::vector<std::uint8_t>& pcm_chunk) const;

    void sendAudio(const std::uint8_t* pcm_samples, std::size_t sample_count) const;

    /// Sends {"type":"flush","flush_id":N}. Server responds with {"type":"flushed","flush_id":N}.
    void sendFlush(int flush_id) const;

    /// Sends {"type":"end_of_stream"}.
    void sendEndOfStream() const;

    void close();

    AsrRealtimeMessage parseMessage(const std::string& raw) const;

private:
    std::string _endpoint;
    std::shared_ptr<transport::IWebSocketTransport> _wsTransport;
    TextMessageCallback   _onMessage;
    ParsedMessageCallback _onParsedMessage;
    ErrorCallback         _onError;
    ClosedCallback        _onClosed;
};

} // namespace gradium
