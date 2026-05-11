# GradiumPP

A C++17 client library for the [Gradium](https://docs.gradium.ai) Text-to-Speech and Automatic Speech Recognition API.

## Features

- REST and WebSocket clients for TTS and ASR
- Voice, pronunciation dictionary, and credits endpoints
- Replaceable HTTP and WebSocket transports for testing or custom integration
- Windows support through WinHTTP and Unix-like platforms through libwebsockets

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

Common API strings are exposed as `constexpr const char*` values in named namespaces. That keeps call sites readable and avoids scattering endpoint or format strings through application code.

Voice IDs are available in `voice_constants.hpp`:

```cpp
config.voice_id = gradium::voices::en::american::emma;
config.voice_id = gradium::voices::fr::french::elise;
config.voice_id = gradium::voices::de::mia;
```

Duplicate names within the same dialect use a `_2` suffix when needed.

TTS formats and models:

```cpp
gradium::tts::output_formats::wav        // default — 48 kHz
gradium::tts::output_formats::pcm
gradium::tts::output_formats::opus
gradium::tts::output_formats::ulaw_8000
gradium::tts::output_formats::alaw_8000
gradium::tts::output_formats::pcm_8000
gradium::tts::output_formats::pcm_16000
gradium::tts::output_formats::pcm_24000
gradium::tts::output_formats::pcm_32000
gradium::tts::output_formats::pcm_48000
gradium::tts::models::default_model
```

ASR formats and models:

```cpp
gradium::asr::input_formats::pcm
gradium::asr::input_formats::wav
gradium::asr::input_formats::opus
gradium::asr::input_formats::ulaw_8000
gradium::asr::input_formats::mulaw_8000
gradium::asr::input_formats::alaw_8000
gradium::asr::input_formats::pcm_8000
gradium::asr::input_formats::pcm_16000
gradium::asr::input_formats::pcm_24000
gradium::asr::input_formats::pcm_32000
gradium::asr::input_formats::pcm_48000
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

See [docs/transport.md](docs/transport.md) for transport behavior and platform notes.

## Endpoint Reference

See [docs/api_coverage.md](docs/api_coverage.md) for the currently implemented endpoints.

## License

Apache 2.0
