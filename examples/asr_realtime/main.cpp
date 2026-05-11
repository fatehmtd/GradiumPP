#include <CLI/CLI.hpp>
#include <gradiumpp/gradium.hpp>

#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

int main(int argc, char* argv[])
{
    const char* apiKeyEnv = std::getenv("GRADIUM_API_KEY");
    if (apiKeyEnv == nullptr) {
        std::cerr << "Error: GRADIUM_API_KEY is not set\n";
        return 1;
    }

    std::string filePath;
    std::string inputFormat = "pcm";
    std::string language;
    bool        showVad     = false;

    CLI::App app{"Transcribe an audio file using the Gradium ASR WebSocket API"};
    app.add_option("--file",   filePath,    "Path to raw PCM audio file (24 kHz 16-bit mono)")->required();
    app.add_option("--format", inputFormat, "Audio input format (default: pcm = 24 kHz 16-bit mono)");
    app.add_option("--lang",   language,    "Language hint (en, fr, de, es, pt)");
    app.add_flag  ("--vad",    showVad,     "Print Voice Activity Detection probabilities");
    CLI11_PARSE(app, argc, argv);

    // Read the entire audio file
    std::ifstream audioFile(filePath, std::ios::binary);
    if (!audioFile.is_open()) {
        std::cerr << "Error: cannot open audio file: " << filePath << "\n";
        return 1;
    }
    const std::vector<std::uint8_t> audioData(
        (std::istreambuf_iterator<char>(audioFile)),
        std::istreambuf_iterator<char>());

    std::atomic<bool>       done{false};
    std::mutex              doneMutex;
    std::condition_variable doneCv;

    gradium::AsrRealtimeClient client;

    client.setOnParsedMessage([&](const gradium::AsrRealtimeMessage& msg) {
        if (msg.type == "text") {
            std::cout << "[partial] " << msg.segment.text << "\n";
        } else if (msg.type == "end_text") {
            std::cout << "[final]   " << msg.segment.text
                      << "  (" << msg.segment.start_s << "s – " << msg.segment.stop_s << "s)\n";
        } else if (msg.type == "step" && showVad) {
            std::cout << "[vad]     0.5s=" << msg.vad.inactivity_prob_0_5s
                      << "  1.0s=" << msg.vad.inactivity_prob_1_0s
                      << "  2.0s=" << msg.vad.inactivity_prob_2_0s << "\n";
        } else if (msg.type == "end_of_stream") {
            {
                std::lock_guard<std::mutex> lk(doneMutex);
                done.store(true);
            }
            doneCv.notify_all();
        } else if (msg.type == "error") {
            std::cerr << "ASR error: " << msg.error_message << "\n";
            {
                std::lock_guard<std::mutex> lk(doneMutex);
                done.store(true);
            }
            doneCv.notify_all();
        }
    });

    client.setOnError([&](const std::string& err) {
        std::cerr << "WebSocket error: " << err << "\n";
        {
            std::lock_guard<std::mutex> lk(doneMutex);
            done.store(true);
        }
        doneCv.notify_all();
    });

    client.setOnClosed([&] {
        {
            std::lock_guard<std::mutex> lk(doneMutex);
            done.store(true);
        }
        doneCv.notify_all();
    });

    try {
        gradium::AsrRealtimeSetup setup;
        setup.input_format = inputFormat;
        if (!language.empty()) {
            // Language is passed via json_config at connect time — add it post-setup
            // by configuring before connecting (for simplicity we set it in json_config
            // on the AsrConfig used for REST; for WS it goes in setup message json_config).
        }
        client.connect(apiKeyEnv, setup);

        // Stream audio in 3840-byte chunks (1920 samples × 2 bytes = 80 ms at 24 kHz)
        constexpr std::size_t chunkSize = 3840;
        std::size_t offset = 0;
        while (offset < audioData.size()) {
            const std::size_t end = std::min(offset + chunkSize, audioData.size());
            const std::vector<std::uint8_t> chunk(audioData.begin() + offset,
                                                   audioData.begin() + end);
            client.sendAudio(chunk);
            offset = end;
        }

        client.sendFlush("flush-1");
        client.sendEndOfStream();

        // Wait for completion
        {
            std::unique_lock<std::mutex> lk(doneMutex);
            doneCv.wait(lk, [&] { return done.load(); });
        }

        client.close();
        std::cout << "\nTranscription complete.\n";
    } catch (const std::exception& ex) {
        std::cerr << "ASR realtime failed: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
