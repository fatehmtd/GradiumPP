#include <CLI/CLI.hpp>
#include <gradiumpp/gradium.hpp>

#include <cppcodec/base64_rfc4648.hpp>

#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

int main(int argc, char* argv[])
{
    const char* apiKeyEnv = std::getenv("GRADIUM_API_KEY");
    if (apiKeyEnv == nullptr) {
        std::cerr << "Error: GRADIUM_API_KEY is not set\n";
        return 1;
    }

    std::string text;
    std::string voiceId;
    std::string outputFormat = "wav";
    std::string outPath      = "tts_realtime_output.wav";

    CLI::App app{"Generate speech from text using the Gradium TTS WebSocket API"};
    app.add_option("--text",   text,         "Text to synthesize")->required();
    app.add_option("--voice",  voiceId,      "Voice UID from the Gradium voice library")->required();
    app.add_option("--format", outputFormat, "Audio output format (wav, pcm, opus, ...)");
    app.add_option("--out",    outPath,      "Output file path");
    CLI11_PARSE(app, argc, argv);

    std::atomic<bool>       ready{false};
    std::mutex              readyMutex;
    std::condition_variable readyCv;

    std::atomic<bool>       done{false};
    std::mutex              doneMutex;
    std::condition_variable doneCv;

    std::ofstream out(outPath, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Error: cannot open output file: " << outPath << "\n";
        return 1;
    }

    gradium::TtsRealtimeClient client;

    client.setOnParsedMessage([&](const gradium::TtsRealtimeMessage& msg) {
        if (msg.type == "ready") {
            {
                std::lock_guard<std::mutex> lk(readyMutex);
                ready.store(true);
            }
            readyCv.notify_all();
            return;
        }

        if (!msg.audio_base64.empty()) {
            auto bytes = cppcodec::base64_rfc4648::decode(msg.audio_base64);
            out.write(reinterpret_cast<const char*>(bytes.data()),
                      static_cast<std::streamsize>(bytes.size()));
        }

        if (msg.type == "end_of_stream") {
            {
                std::lock_guard<std::mutex> lk(doneMutex);
                done.store(true);
            }
            doneCv.notify_all();
        }

        if (msg.type == "error") {
            std::cerr << "TTS error: " << msg.error_message << "\n";
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
        client.connect(apiKeyEnv);

        gradium::TtsRealtimeSetup setup;
        setup.voice_id      = voiceId;
        setup.output_format = outputFormat;
        client.setup(setup);

        // Wait for server "ready" before sending text
        {
            std::unique_lock<std::mutex> lk(readyMutex);
            readyCv.wait(lk, [&] { return ready.load(); });
        }

        client.sendText(text);
        client.sendEndOfStream();

        // Wait for completion
        {
            std::unique_lock<std::mutex> lk(doneMutex);
            doneCv.wait(lk, [&] { return done.load(); });
        }

        out.close();
        client.close();

        std::cout << "Wrote realtime TTS output to " << outPath << "\n";
    } catch (const std::exception& ex) {
        std::cerr << "TTS realtime failed: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
