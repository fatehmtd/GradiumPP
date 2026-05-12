#include <CLI/CLI.hpp>
#include <gradiumpp/gradium.hpp>

#include <algorithm>
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
    bool        showVad     = false;

    CLI::App app{"Transcribe an audio file over the Gradium ASR WebSocket API"};
    app.add_option("--file",   filePath,    "Path to a raw PCM file (24 kHz, 16-bit, mono)")->required();
    app.add_option("--format", inputFormat, "Input format");
    app.add_flag  ("--vad",    showVad,     "Print VAD probabilities");
    CLI11_PARSE(app, argc, argv);

    std::ifstream audioFile(filePath, std::ios::binary);
    if (!audioFile.is_open()) {
        std::cerr << "Error: cannot open audio file: " << filePath << "\n";
        return 1;
    }
    const std::vector<std::uint8_t> audioData(
        (std::istreambuf_iterator<char>(audioFile)),
        std::istreambuf_iterator<char>());

    std::atomic<bool>       ready{false};
    std::mutex              readyMutex;
    std::condition_variable readyCv;

    std::atomic<bool>       done{false};
    std::atomic<bool>       failed{false};
    std::mutex              doneMutex;
    std::condition_variable doneCv;

    gradium::AsrRealtimeClient client;

    client.setOnParsedMessage([&](const gradium::AsrRealtimeMessage& msg) {
        if (msg.type == "ready") {
            {
                std::lock_guard<std::mutex> lk(readyMutex);
                ready.store(true);
            }
            readyCv.notify_all();
        } else if (msg.type == "text") {
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
                failed.store(true);
                done.store(true);
            }
            doneCv.notify_all();
        }
    });

    client.setOnError([&](const std::string& err) {
        std::cerr << "WebSocket error: " << err << "\n";
        {
            std::lock_guard<std::mutex> lk(doneMutex);
            failed.store(true);
            done.store(true);
        }
        doneCv.notify_all();
    });

    client.setOnClosed([&] {
        {
            std::lock_guard<std::mutex> lk(doneMutex);
            if (!done.load()) {
                failed.store(true);
                done.store(true);
            }
        }
        doneCv.notify_all();
    });

    try {
        gradium::AsrRealtimeSetup setup;
        setup.input_format = inputFormat;
        client.connect(apiKeyEnv, setup);

        {
            std::unique_lock<std::mutex> lk(readyMutex);
            readyCv.wait(lk, [&] { return ready.load() || failed.load(); });
        }

        if (failed.load()) {
            throw std::runtime_error("[gradiumpp] ASR session ended before ready");
        }

        constexpr std::size_t chunkSize = 3840;
        std::size_t offset = 0;
        while (offset < audioData.size()) {
            const std::size_t end = std::min(offset + chunkSize, audioData.size());
            const std::vector<std::uint8_t> chunk(audioData.begin() + offset,
                                                   audioData.begin() + end);
            client.sendAudio(chunk);
            offset = end;
        }

        client.sendEndOfStream();

        {
            std::unique_lock<std::mutex> lk(doneMutex);
            doneCv.wait(lk, [&] { return done.load(); });
        }

        client.close();
        if (failed.load()) {
            std::cerr << "\nTranscription did not complete successfully.\n";
            return 1;
        }

        std::cout << "\nTranscription complete.\n";
    } catch (const std::exception& ex) {
        std::cerr << "ASR realtime failed: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
