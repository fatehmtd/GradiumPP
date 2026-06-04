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

struct Voice {
    std::string uid;
    std::string name;
    std::string description;
    std::string filename;
    double      start_s{0.0};
    bool        is_catalog{false};
    bool        is_pro_clone{false};
    std::string language;
    std::vector<std::string> tags;
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

struct TtsJsonConfig {
    double      temp{0.0};          ///< 0.0 = use server default (0.7)
    double      cfg_coef{0.0};      ///< 0.0 = use server default (2.0)
    double      padding_bonus{0.0}; ///< negative = faster, positive = slower
    std::vector<std::string> rewrite_rules; ///< language codes e.g. {"en"}
    std::string pronunciation_id;   ///< optional pronunciation dictionary UID
};

struct TtsConfig {
    std::string   voice_id;
    std::string   model_name{tts::models::default_model};
    std::string   output_format{tts::output_formats::wav};
    bool          only_audio{true};
    TtsJsonConfig json_config;
};

// ---------------------------------------------------------------------------
// ASR configuration
// ---------------------------------------------------------------------------

struct AsrJsonConfig {
    double      temp{0.0};
    std::string language;           ///< hint: "en", "fr", "de", "es", "pt"
    double      padding_bonus{0.0};
    int         delay_in_frames{0}; ///< 0 = server default; values: 7,8,10,12,...
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
    std::string session_id;     ///< populated on "ready"
    std::string audio_base64;   ///< base64-encoded audio chunk; populated on "audio"
    std::string text;           ///< populated on "text"
    double      start_s{0.0};   ///< populated on "text"
    std::string stream_id;
    std::string client_req_id;  ///< echoed from sent messages (multiplexing)
    std::string error_message;  ///< populated on "error"
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
    std::string session_id;     ///< on "ready"
    int         sample_rate{0}; ///< on "ready"; expected input sample rate (24 000)
    int         frame_size{0};  ///< on "ready"; expected samples per chunk (1920)
    VadStep     vad;            ///< on "step"
    TranscriptSegment segment;  ///< on "text" and "end_text"
    std::string flush_id;       ///< on "flushed"
    std::string client_req_id;  ///< echoed from sent messages (multiplexing)
    std::string error_message;  ///< on "error"
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
