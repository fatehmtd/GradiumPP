# Endpoint Reference

This library currently covers the following Gradium endpoints, verified against
Gradium's published OpenAPI spec (`https://docs.gradium.ai/api-reference/openapi.json`).

## TTS

- `POST /post/speech/tts` through `TtsRestClient::generateSpeech()`. The request body
  is `text`, `voice_id`, `output_format`, `only_audio` only — this endpoint does not
  accept `model_name` or `json_config`; those are WebSocket-only (see below).
- `wss://api.gradium.ai/api/speech/tts` through `TtsRealtimeClient`

## ASR

- `POST /post/speech/asr` through `AsrRestClient::transcribe()` and `transcribeFile()`.
  Query parameters are `model`, `input_format` (`wav`/`pcm`/`opus` only), and
  `json_config`. Content-Type is `audio/wav`, `audio/pcm`, or `audio/ogg` (Ogg-wrapped
  Opus — not `audio/opus`).
- `wss://api.gradium.ai/api/speech/asr` through `AsrRealtimeClient`

## S2S (speech-to-speech)

- `wss://api.gradium.ai/api/speech/s2s` through `S2sRealtimeClient`. Incoming audio is
  transcribed, translated, and re-synthesized with the configured `voice_id`.
- `"s2s-translate"` is the only `model_name` the API currently accepts (the default in
  `S2sRealtimeSetup`); it requires `json_config.target_language` — setup is rejected
  with `{"type":"error","message":"missing required 'language'", ...}` without it.
  Defaults for `stt_model_name`/`tts_model_name` (`"stt-translate"`/`"default"`) match
  the values Gradium's own docs recommend for this model.

## Voices

- `GET /voices/` through `VoiceClient::listVoices()` and `listVoicesTyped()`
- `POST /voices/` through `VoiceClient::createVoice()` and `createVoiceTyped()`.
  The endpoint itself only returns `{uid, error, was_updated}` —
  `createVoiceTyped()` throws on a non-empty `error`, then issues a follow-up
  `GET /voices/{uid}` to return the full `Voice`.
- `GET /voices/{uid}` through `VoiceClient::getVoice()` and `getVoiceTyped()`
- `PUT /voices/{uid}` through `VoiceClient::updateVoice()` and `updateVoiceTyped()`
- `DELETE /voices/{uid}` through `VoiceClient::deleteVoice()`

Voice tags are `{category, value}` objects (`gradium::VoiceTag`), not plain strings.
`POST /voices/` does not accept a `tags` field — tags are set through
`PUT /voices/{uid}` (`VoiceUpdateRequest::tags`) after creation.

## Pronunciation dictionaries

- `GET /pronunciations/` through `PronunciationClient::listDictionaries()` and `listDictionariesTyped()`
- `POST /pronunciations/` through `PronunciationClient::createDictionary()` and `createDictionaryTyped()`
- `GET /pronunciations/{uid}` through `PronunciationClient::getDictionary()` and `getDictionaryTyped()`
- `PUT /pronunciations/{uid}` through `PronunciationClient::updateDictionary()` and `updateDictionaryTyped()`
- `DELETE /pronunciations/{uid}` through `PronunciationClient::deleteDictionary()`

## Credits

- `GET /usages/credits` through `CreditsClient::getCredits()` and `getCreditsTyped()`

## Real-time behavior

Supported message flow today includes:

- TTS setup, ready, text, audio, and end-of-stream messages, including `json_config`
  (temp, cfg_coef, padding_bonus, rewrite_rules, pronunciation_id) and `retry_for_s`
  on the setup message
- ASR setup, audio, VAD steps, partial text, final text, flush, and end-of-stream
  messages, including `json_config` (temp, language, target_language, padding_bonus,
  delay_in_frames, keyword boosting) and `retry_for_s` on the setup message
- S2S setup, audio in/out, text (transcript), and end-of-stream messages
- Multiplexing through `client_req_id` on TTS, ASR, and S2S
- Optional keep-alive behavior with `close_ws_on_eos = false`
- `flush_id` is an integer on the wire (`AsrRealtimeClient::sendFlush(int)`), matching
  the server's `{"type":"flush","flush_id":N}` / `{"type":"flushed","flush_id":N}` contract
- `ready` messages report `request_id` (not `session_id`); `error` messages report a
  numeric `code` (e.g. `1002`, `1008`, `1011`) alongside `message`

Endpoint constants live in `api_constants.hpp` if you want to reference the library defaults directly.

## Known gaps

- `voice_constants.hpp` mirrors Gradium's current 66-voice "flagship" list
  (`/guides/voices/flagship-voices`), not the full catalog — use
  `VoiceClient::listVoicesTyped(true)` for every voice available to your account.
- REST error bodies for TTS/ASR (`POST /post/speech/tts` and `/asr`) are plain text
  on pre-stream failures (e.g. `error from server 1008: ...`), not JSON. `throwIfError()`
  attempts JSON parsing for all clients; for these two REST clients a plain-text body
  ends up in `GradiumErrorDetail::raw_body` with `message`/`code` left empty rather than
  the parsed Gradium error code.
