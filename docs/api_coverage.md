# GradiumPP — API Coverage

## TTS Endpoints

| Endpoint | Method | Client Class | Coverage |
|----------|--------|--------------|----------|
| `/post/speech/tts` | POST | `TtsRestClient::generateSpeech()` | ✅ |
| `wss://.../speech/tts` | WebSocket | `TtsRealtimeClient` | ✅ |

## ASR Endpoints

| Endpoint | Method | Client Class | Coverage |
|----------|--------|--------------|----------|
| `/post/speech/asr` | POST | `AsrRestClient::transcribe()` / `transcribeFile()` | ✅ |
| `wss://.../speech/asr` | WebSocket | `AsrRealtimeClient` | ✅ |

## Voice Management

| Endpoint | Method | Client Class | Coverage |
|----------|--------|--------------|----------|
| `/voices/` | GET | `VoiceClient::listVoices()` / `listVoicesTyped()` | ✅ |
| `/voices/` | POST | `VoiceClient::createVoice()` / `createVoiceTyped()` | ✅ |
| `/voices/{uid}` | GET | `VoiceClient::getVoice()` / `getVoiceTyped()` | ✅ |
| `/voices/{uid}` | PUT | `VoiceClient::updateVoice()` / `updateVoiceTyped()` | ✅ |
| `/voices/{uid}` | DELETE | `VoiceClient::deleteVoice()` | ✅ |

## Pronunciation Dictionaries

| Endpoint | Method | Client Class | Coverage |
|----------|--------|--------------|----------|
| `/pronunciations/` | GET | `PronunciationClient::listDictionaries()` / `listDictionariesTyped()` | ✅ |
| `/pronunciations/` | POST | `PronunciationClient::createDictionary()` / `createDictionaryTyped()` | ✅ |
| `/pronunciations/{uid}` | GET | `PronunciationClient::getDictionary()` / `getDictionaryTyped()` | ✅ |
| `/pronunciations/{uid}` | PUT | `PronunciationClient::updateDictionary()` / `updateDictionaryTyped()` | ✅ |
| `/pronunciations/{uid}` | DELETE | `PronunciationClient::deleteDictionary()` | ✅ |

## Usage / Credits

| Endpoint | Method | Client Class | Coverage |
|----------|--------|--------------|----------|
| `/usages/credits` | GET | `CreditsClient::getCredits()` / `getCreditsTyped()` | ✅ |

## WebSocket Features

| Feature | Supported |
|---------|-----------|
| TTS setup → ready → text → audio | ✅ |
| ASR setup → audio → VAD step → text/end_text | ✅ |
| ASR flush / flushed | ✅ |
| Multiplexing via `client_req_id` | ✅ |
| `close_ws_on_eos = false` (keep-alive) | ✅ |
| Base64 audio encoding (ASR send) | ✅ (internal) |
| Base64 audio decoding (TTS receive) | caller-side (cppcodec in examples) |

## Endpoint Constants

All endpoint paths are declared as `constexpr const char*` in `api_constants.hpp` — no raw strings in client code:

```cpp
gradium::tts::endpoints::rest        // "/post/speech/tts"
gradium::tts::endpoints::realtime    // "wss://api.gradium.ai/api/speech/tts"
gradium::asr::endpoints::rest        // "/post/speech/asr"
gradium::asr::endpoints::realtime    // "wss://api.gradium.ai/api/speech/asr"
gradium::voices::endpoints::base     // "/voices/"
gradium::pronunciations::endpoints::base  // "/pronunciations/"
gradium::credits::endpoints::base    // "/usages/credits"
gradium::api::base_url               // "https://api.gradium.ai/api"
```
