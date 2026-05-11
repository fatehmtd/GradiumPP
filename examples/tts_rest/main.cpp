#include <CLI/CLI.hpp>
#include <gradiumpp/gradium.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    const char* apiKeyEnv = std::getenv("GRADIUM_API_KEY");
    if (apiKeyEnv == nullptr) {
        std::cerr << "Error: GRADIUM_API_KEY is not set\n";
        return 1;
    }

    std::string text = "Setting off on an adventure, I ventured into the unknown. "
                       "Every step brought new discoveries, every turn revealed something unexpected. "
                       "The world was full of possibilities waiting to be explored.";
    std::string voiceId = gradium::voices::en::american::abigail;
    std::string outputFormat = "wav";
    std::string outPath      = "tts_output.wav";

    CLI::App app{"Generate speech from text using the Gradium TTS REST API"};
    app.add_option("--text",   text,         "Text to synthesize");
    app.add_option("--voice",  voiceId,      "Voice UID from the Gradium voice library");
    app.add_option("--format", outputFormat, "Audio output format (wav, pcm, opus, ...)");
    app.add_option("--out",    outPath,      "Output file path");
    CLI11_PARSE(app, argc, argv);

    gradium::TtsRestClient client(apiKeyEnv);

    gradium::TtsConfig config;
    config.voice_id      = voiceId;
    config.output_format = outputFormat;
    config.only_audio    = true;

    try {
        const auto response = client.generateSpeech(config, text);

        std::ofstream out(outPath, std::ios::binary);
        if (!out.is_open()) {
            std::cerr << "Error: cannot open output file: " << outPath << "\n";
            return 1;
        }

        out.write(reinterpret_cast<const char*>(response.body.data()),
                  static_cast<std::streamsize>(response.body.size()));
        out.close();

        std::cout << "Wrote " << response.body.size() << " bytes to " << outPath << "\n";
    } catch (const gradium::GradiumApiException& ex) {
        std::cerr << "API error: " << ex.what() << "\n";
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
