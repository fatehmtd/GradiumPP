#pragma once

// ── Base URL ──────────────────────────────────────────────────────────────────

namespace gradium::api {

constexpr const char* base_url = "https://api.gradium.ai/api";

} // namespace gradium::api

// ── TTS ───────────────────────────────────────────────────────────────────────

namespace gradium::tts {

namespace models {

constexpr const char* default_model = "default";

} // namespace models

/* Default sample rate is 48 000 Hz for wav/pcm/opus.
   Telephony variants (ulaw_8000, mulaw_8000, alaw_8000) are fixed at 8 000 Hz.
   The pcm_* variants fix the sample rate to the value in their name. */
namespace output_formats {

constexpr const char* wav       = "wav";
constexpr const char* pcm       = "pcm";
constexpr const char* opus      = "opus";
constexpr const char* ulaw_8000  = "ulaw_8000";
constexpr const char* mulaw_8000 = "mulaw_8000";
constexpr const char* alaw_8000  = "alaw_8000";
constexpr const char* pcm_8000  = "pcm_8000";
constexpr const char* pcm_16000 = "pcm_16000";
constexpr const char* pcm_22050 = "pcm_22050";
constexpr const char* pcm_24000 = "pcm_24000";
constexpr const char* pcm_44100 = "pcm_44100";
constexpr const char* pcm_48000 = "pcm_48000";

} // namespace output_formats

namespace endpoints {

constexpr const char* rest     = "/post/speech/tts";
constexpr const char* realtime = "wss://api.gradium.ai/api/speech/tts";

} // namespace endpoints

} // namespace gradium::tts

// ── ASR ───────────────────────────────────────────────────────────────────────

namespace gradium::asr {

namespace models {

constexpr const char* default_model = "default";

} // namespace models

/* Default input format is pcm (24 kHz, 16-bit signed little-endian mono).
   Expected frame size for real-time is 1920 samples (80 ms).
   Note: the REST endpoint (POST /post/speech/asr) only accepts "wav", "pcm",
   and "opus" for its input_format query parameter; the other formats below
   are only valid on the real-time WebSocket setup message. */
namespace input_formats {

constexpr const char* pcm      = "pcm";
constexpr const char* pcm_8000  = "pcm_8000";
constexpr const char* pcm_16000 = "pcm_16000";
constexpr const char* pcm_22050 = "pcm_22050";
constexpr const char* pcm_24000 = "pcm_24000";
constexpr const char* pcm_44100 = "pcm_44100";
constexpr const char* pcm_48000 = "pcm_48000";
constexpr const char* wav       = "wav";
constexpr const char* opus      = "opus";
constexpr const char* ulaw_8000  = "ulaw_8000";
constexpr const char* mulaw_8000 = "mulaw_8000";
constexpr const char* alaw_8000  = "alaw_8000";

} // namespace input_formats

namespace endpoints {

constexpr const char* rest     = "/post/speech/asr";
constexpr const char* realtime = "wss://api.gradium.ai/api/speech/asr";

} // namespace endpoints

} // namespace gradium::asr

// ── S2S ───────────────────────────────────────────────────────────────────────

namespace gradium::s2s {

namespace models {

// "s2s-translate" is the only model currently supported by the S2S endpoint;
// it always requires json_config.target_language on setup (see S2sRealtimeSetup).
constexpr const char* default_model     = "s2s-translate";
constexpr const char* default_stt_model = "stt-translate";
constexpr const char* default_tts_model = "default";

} // namespace models

namespace endpoints {

constexpr const char* realtime = "wss://api.gradium.ai/api/speech/s2s";

} // namespace endpoints

} // namespace gradium::s2s

// ── Voices ────────────────────────────────────────────────────────────────────

namespace gradium::voices::endpoints {

constexpr const char* base = "/voices/";

} // namespace gradium::voices::endpoints

// ── Pronunciations ────────────────────────────────────────────────────────────

namespace gradium::pronunciations::endpoints {

constexpr const char* base = "/pronunciations/";

} // namespace gradium::pronunciations::endpoints

// ── Credits ───────────────────────────────────────────────────────────────────

namespace gradium::credits::endpoints {

constexpr const char* base = "/usages/credits";

} // namespace gradium::credits::endpoints
