#pragma once

#include <gradiumpp/api_constants.hpp>

#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace gradium {

constexpr const char* USER_AGENT = "gradiumpp/1.0";

// ---------------------------------------------------------------------------
// Voice
// ---------------------------------------------------------------------------

struct VoiceTag {
    std::string category; ///< optional; empty when the tag has no category
    std::string value;
};

struct Voice {
    std::string uid;
    std::string name;
    std::string description;
    std::string filename;
    double      start_s{0.0};
    double      stop_s{0.0};      ///< populated on GET/PUT responses; not on the catalog list
    bool        is_catalog{false};
    bool        is_pro_clone{false};
    bool        is_pending{false}; ///< true while an async voice clone is still processing
    bool        has_audio{true};
    std::string language;
    std::string org_uid;
    std::vector<VoiceTag> tags;
};

// ---------------------------------------------------------------------------
// Pronunciation Dictionary
// ---------------------------------------------------------------------------

struct PronunciationRule {
    std::string id;
    std::string original;
    std::string rewrite;
    bool        case_sensitive{false};
};

struct PronunciationDictionary {
    std::string uid;
    std::string org_uid;
    std::string name;
    std::string description;
    std::string language;
    std::string created_at;
    std::vector<PronunciationRule> rules;
};

// ---------------------------------------------------------------------------
// Credits / Usage
// ---------------------------------------------------------------------------

struct CreditsSummary {
    double      remaining_credits{0.0};
    double      allocated_credits{0.0};
    std::string billing_period;
    std::string next_rollover_date;
    std::string plan_name;
};

// ---------------------------------------------------------------------------
// TTS configuration
// ---------------------------------------------------------------------------

/// Advanced TTS settings; real-time WebSocket only, not accepted by the REST endpoint.
struct TtsJsonConfig {
    double      temp{0.0};          ///< 0.0 = use server default (0.7); range 0.0-1.4
    double      cfg_coef{0.0};      ///< 0.0 = use server default (2.0); range 1.0-4.0 (voice similarity)
    double      padding_bonus{0.0}; ///< negative = faster, positive = slower; range -4.0-4.0
    std::vector<std::string> rewrite_rules; ///< language codes e.g. {"en"}
    std::string pronunciation_id;   ///< optional pronunciation dictionary UID
};

/// Body for POST /post/speech/tts — text, voice_id, output_format, only_audio only.
struct TtsConfig {
    std::string voice_id;
    std::string output_format{tts::output_formats::wav};
    bool        only_audio{true};
};

// ---------------------------------------------------------------------------
// ASR configuration
// ---------------------------------------------------------------------------

/// Bias transcription toward a custom vocabulary (up to 500 keywords).
struct AsrKeywordBoost {
    std::vector<std::string> words;
    double boost{3.0}; ///< log-space bias, range -6.0 to 6.0; 3.0 is the recommended default
};

struct AsrJsonConfig {
    double      temp{0.0};            ///< 0.0-1.5; 0.0 is greedy decoding
    std::string language;             ///< hint: "en", "fr", "de", "es", "pt"
    std::string target_language;      ///< output language for translating models only
    double      padding_bonus{0.0};   ///< -4.0-4.0; biases emission timing sooner/later
    int         delay_in_frames{0};   ///< 0 = server default; valid range 0-80
    AsrKeywordBoost keywords;         ///< empty words = no keyword boosting sent
};

struct AsrConfig {
    std::string   input_format{asr::input_formats::wav};
    std::string   model_name{asr::models::default_model};
    AsrJsonConfig json_config;
};

// ---------------------------------------------------------------------------
// WebSocket TTS server messages
// ---------------------------------------------------------------------------

/**
 * TTS real-time WebSocket message types.
 */
namespace tts::message {
    constexpr const char* READY = "ready";
    constexpr const char* AUDIO = "audio";
    constexpr const char* TEXT  = "text";
    constexpr const char* END_OF_STREAM = "end_of_stream";
    constexpr const char* ERROR = "error";
}

/// Parsed server message received on the TTS real-time WebSocket.
struct TtsRealtimeMessage {
    enum class Type { UNKNOWN, Ready, Audio, Text, EndOfStream, Error } type;
    std::string type_str;           ///< "ready", "audio", "text", "end_of_stream", "error"
    std::string request_id;     ///< populated on "ready"
    int         sample_rate{0}; ///< on "ready"; output sample rate (48 000 by default)
    int         frame_size{0};  ///< on "ready"; output samples per chunk
    std::string audio_base64;   ///< base64-encoded audio chunk; populated on "audio"
    std::string text;           ///< populated on "text"
    double      start_s{0.0};   ///< populated on "text"
    double      stop_s{0.0};    ///< populated on "text"
    std::string stream_id;
    std::string client_req_id;  ///< echoed from sent messages (multiplexing)
    std::string error_message;  ///< populated on "error"
    int         error_code{0};  ///< populated on "error", e.g. 1008, 1011
    std::string raw_message;
};

// ---------------------------------------------------------------------------
// WebSocket ASR server messages
// ---------------------------------------------------------------------------

/// Voice Activity Detection step — emitted every 80 ms during transcription.
struct VadStep {
    double inactivity_prob_0_5s{0.0}; ///< 0.5 s horizon prediction
    double inactivity_prob_1_0s{0.0}; ///< 1.0 s horizon prediction
    double inactivity_prob_2_0s{0.0}; ///< 2.0 s horizon (recommended for turn-taking)
};

/// A transcribed text segment (either partial "text" or final "end_text").
struct TranscriptSegment {
    std::string text;
    double      start_s{0.0};
    double      stop_s{0.0};  ///< only populated on "end_text"
    std::string stream_id;
};

/**
 * ASR real-time WebSocket message types.
 */
namespace stt::message {
    constexpr const char* READY = "ready";
    constexpr const char* STEP  = "step";
    constexpr const char* TEXT  = "text";
    constexpr const char* END_TEXT = "end_text";
    constexpr const char* FLUSHED = "flushed";
    constexpr const char* END_OF_STREAM = "end_of_stream";
    constexpr const char* ERROR = "error";
}

/// Parsed server message received on the ASR real-time WebSocket.
struct AsrRealtimeMessage {
    enum class Type { UNKNOWN, Ready, Step, Text, EndText, Flushed, EndOfStream, Error } type;
    std::string type_str;           ///< "ready","step","text","end_text","flushed","end_of_stream","error"
    std::string request_id;     ///< on "ready"
    int         sample_rate{0}; ///< on "ready"; expected input sample rate (24 000)
    int         frame_size{0};  ///< on "ready"; expected samples per chunk (1920)
    int         delay_in_frames{0}; ///< on "ready"; active adaptive-delay setting
    VadStep     vad;            ///< on "step"
    TranscriptSegment segment;  ///< on "text" and "end_text"
    int         flush_id{0};    ///< on "flushed"; echoes the client's flush_id
    std::string client_req_id;  ///< echoed from sent messages (multiplexing)
    std::string error_message;  ///< on "error"
    int         error_code{0};  ///< on "error", e.g. 1002, 1008, 1011
    std::string raw_message;
};

// ---------------------------------------------------------------------------
// Error / Exception
// ---------------------------------------------------------------------------

struct GradiumErrorDetail {
    int         status_code{0};
    std::string message;
    std::string code;
    std::string raw_body;
};

/// Thrown for Gradium REST API errors (HTTP >= 400).
class GradiumApiException : public std::runtime_error {
public:
    GradiumApiException(std::string operation, GradiumErrorDetail detail)
        : std::runtime_error(makeWhat(operation, detail)),
          operation_(std::move(operation)),
          detail_(std::move(detail))
    {
    }

    const std::string&        operation() const noexcept { return operation_; }
    const GradiumErrorDetail& detail()    const noexcept { return detail_; }

private:
    static std::string makeWhat(const std::string& op, const GradiumErrorDetail& d)
    {
        std::string out = "[gradiumpp] " + op + " failed with status "
                        + std::to_string(d.status_code);
        if (!d.code.empty())    out += ", code="    + d.code;
        if (!d.message.empty()) out += ", message=" + d.message;
        return out;
    }

    std::string        operation_;
    GradiumErrorDetail detail_;
};

} // namespace gradium
