# GradiumPP

A C++17 client library for the [Gradium](https://docs.gradium.ai) Text-to-Speech and Automatic Speech Recognition API.

## Features

- **TTS REST** — single HTTP POST, returns raw audio bytes
- **TTS WebSocket** — real-time streaming with sub-second latency; multiplexing support
- **ASR REST** — transcribe pre-recorded audio files
- **ASR WebSocket** — real-time streaming with Voice Activity Detection (VAD)
- **Voice Management** — list, create, update, and delete custom voices
- **Pronunciation Dictionaries** — manage custom pronunciation rules
- **Credits/Usage** — query remaining API credits
- **Pluggable transport** — inject custom HTTP/WebSocket backends for testing
- **Multi-platform** — Windows (WinHTTP), Linux/macOS (libwebsockets)

## Requirements

- C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.16+
- Internet access for FetchContent (dependencies downloaded automatically)

## Building

```bash
cmake -B build
cmake --build build
```

To skip building examples:

```bash
cmake -B build -DGRADIUM_BUILD_EXAMPLES=OFF
cmake --build build
```

## Quick Start

### TTS REST

```cpp
#include <gradiumpp/gradium.hpp>
#include <fstream>

int main()
{
    gradium::TtsRestClient client("gd_your_api_key_here");

    gradium::TtsConfig config;
    config.voice_id      = gradium::voices::en::american::abigail;
    config.output_format = gradium::tts::output_formats::wav;
    config.only_audio    = true;

    auto response = client.generateSpeech(config, "Hello, world!");

    std::ofstream out("output.wav", std::ios::binary);
    out.write(reinterpret_cast<const char*>(response.body.data()),
              response.body.size());
}
```

### ASR REST

```cpp
#include <gradiumpp/gradium.hpp>
#include <iostream>

int main()
{
    gradium::AsrRestClient client("gd_your_api_key_here");

    gradium::AsrConfig config;
    config.input_format = gradium::asr::input_formats::wav;

    std::string transcript = client.transcribeFile("recording.wav", config);
    std::cout << transcript << "\n";
}
```

### Custom Transport Injection

```cpp
auto myHttp = std::make_shared<MyMockHttpTransport>();
gradium::TtsRestClient client("api-key", myHttp);
```

## Constants

All string constants are `constexpr const char*` in named namespaces — no raw string literals needed in user code.

### Voice IDs (`voice_constants.hpp`)

237 catalog voices organized by language and dialect:

```text
gradium::voices::en::american   — 71 voices  (emma, abigail, john, …)
gradium::voices::en::british    — 21 voices  (eva, jack, kelly, …)
gradium::voices::en::australian —  6 voices  (hunter, samuel, …)
gradium::voices::en::other      —  4 voices  (arjun, michelle, …)
gradium::voices::fr::french     — 40 voices  (elise, leo, sarah, …)
gradium::voices::fr::canadian   —  3 voices  (melanie, maxime, …)
gradium::voices::de             — 50 voices  (mia, maximilian, …)
gradium::voices::es::spanish    — 11 voices  (sergio, sofia, …)
gradium::voices::es::mexican    —  6 voices  (valentina, adrian, …)
gradium::voices::es::other      —  3 voices  (javier, carmen, …)
gradium::voices::pt::brazilian  — 17 voices  (alice, davi, …)
gradium::voices::pt::portuguese —  7 voices  (rodrigo, bruna, …)
```

Usage:

```cpp
config.voice_id = gradium::voices::en::american::emma;
config.voice_id = gradium::voices::fr::french::elise;
config.voice_id = gradium::voices::de::mia;
```

Duplicate names within the same dialect are disambiguated with a `_2` suffix
(e.g. `gradium::voices::fr::french::sarah` and `gradium::voices::fr::french::sarah_2`).

### TTS Formats & Models (`api_constants.hpp`)

```cpp
// Output formats
gradium::tts::output_formats::wav        // default — 48 kHz
gradium::tts::output_formats::pcm
gradium::tts::output_formats::opus
gradium::tts::output_formats::ulaw_8000  // telephony, fixed 8 kHz
gradium::tts::output_formats::alaw_8000
gradium::tts::output_formats::pcm_8000   // fixed sample-rate variants
gradium::tts::output_formats::pcm_16000
gradium::tts::output_formats::pcm_24000
gradium::tts::output_formats::pcm_32000
gradium::tts::output_formats::pcm_48000

// Model
gradium::tts::models::default_model
```

### ASR Formats & Models (`api_constants.hpp`)

```cpp
// Input formats
gradium::asr::input_formats::pcm         // default — 24 kHz 16-bit LE mono
gradium::asr::input_formats::wav
gradium::asr::input_formats::opus
gradium::asr::input_formats::ulaw_8000
gradium::asr::input_formats::mulaw_8000
gradium::asr::input_formats::alaw_8000
gradium::asr::input_formats::pcm_8000    // fixed sample-rate variants
gradium::asr::input_formats::pcm_16000
gradium::asr::input_formats::pcm_24000
gradium::asr::input_formats::pcm_32000
gradium::asr::input_formats::pcm_48000

// Model
gradium::asr::models::default_model
```

## Environment Variable

All examples read the API key from `GRADIUM_API_KEY`:

```bash
export GRADIUM_API_KEY=gd_your_api_key_here
```

## Examples

| Example | Description |
|---------|-------------|
| `gradium_tts_rest` | REST TTS → write audio file |
| `gradium_tts_realtime` | WebSocket TTS with ready/audio/end_of_stream flow |
| `gradium_tts_multiplex` | Three concurrent streams on one WebSocket |
| `gradium_asr_rest` | REST ASR → print NDJSON transcript |
| `gradium_asr_realtime` | WebSocket ASR with live transcript + optional VAD output |
| `gradium_voices` | List voices, create/update/delete custom voice, check credits |

## Transport Architecture

See [docs/transport.md](docs/transport.md) for details on the pluggable transport layer.

## API Coverage

See [docs/api_coverage.md](docs/api_coverage.md) for the full endpoint coverage matrix.

## License

Apache 2.0
