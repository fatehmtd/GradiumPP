# Endpoint Reference

This library currently covers the following Gradium endpoints.

## TTS

- `POST /post/speech/tts` through `TtsRestClient::generateSpeech()`
- `wss://api.gradium.ai/api/speech/tts` through `TtsRealtimeClient`

## ASR

- `POST /post/speech/asr` through `AsrRestClient::transcribe()` and `transcribeFile()`
- `wss://api.gradium.ai/api/speech/asr` through `AsrRealtimeClient`

## Voices

- `GET /voices/` through `VoiceClient::listVoices()` and `listVoicesTyped()`
- `POST /voices/` through `VoiceClient::createVoice()` and `createVoiceTyped()`
- `GET /voices/{uid}` through `VoiceClient::getVoice()` and `getVoiceTyped()`
- `PUT /voices/{uid}` through `VoiceClient::updateVoice()` and `updateVoiceTyped()`
- `DELETE /voices/{uid}` through `VoiceClient::deleteVoice()`

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

- TTS setup, ready, text, audio, and end-of-stream messages
- ASR setup, audio, VAD steps, partial text, final text, flush, and end-of-stream messages
- Multiplexing through `client_req_id`
- Optional keep-alive behavior with `close_ws_on_eos = false`

Endpoint constants live in `api_constants.hpp` if you want to reference the library defaults directly.
