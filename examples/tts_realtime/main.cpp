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
    std::string outPath = "tts_realtime_output.wav";

    CLI::App app{ "Generate speech over the Gradium TTS WebSocket API" };
    app.add_option("--text", text, "Text to synthesize")->required();
    app.add_option("--voice", voiceId, "Voice ID")->required();
    app.add_option("--format", outputFormat, "Output format");
    app.add_option("--out", outPath, "Output file path");
    CLI11_PARSE(app, argc, argv);

    std::atomic<bool>       ready{ false };
    std::mutex              readyMutex;
    std::condition_variable readyCv;

    std::atomic<bool>       done{ false };
    std::atomic<bool>       failed{ false };
    std::mutex              doneMutex;
    std::condition_variable doneCv;

    std::ofstream out(outPath, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Error: cannot open output file: " << outPath << "\n";
        return 1;
    }

    gradium::TtsRealtimeClient client;

    client.setOnParsedMessage([&](const gradium::TtsRealtimeMessage& msg) {

        switch (msg.type) {
        case gradium::TtsRealtimeMessage::Type::Ready:
        {
            {
                std::lock_guard<std::mutex> lk(readyMutex);
                ready.store(true);
            }
            readyCv.notify_all();
            std::cout << "TTS session is ready. Request ID: " << msg.request_id << " Raw message: " << msg.raw_message << "\n";
        } break;
        case gradium::TtsRealtimeMessage::Type::Audio:
        {
            if (!msg.audio_base64.empty()) {
                auto bytes = cppcodec::base64_rfc4648::decode(msg.audio_base64);
                out.write(reinterpret_cast<const char*>(bytes.data()),
                    static_cast<std::streamsize>(bytes.size()));
            }
            else {
                std::cerr << "Warning: received audio message with empty audio_base64\n";
            }
        } break;
        case gradium::TtsRealtimeMessage::Type::EndOfStream:
        {
            {
                std::lock_guard<std::mutex> lk(doneMutex);
                done.store(true);
            }
            doneCv.notify_all();
        } break;
        case gradium::TtsRealtimeMessage::Type::Error:
        {
            // Wake the ready-wait too — an error can arrive before "ready".
            std::cerr << "TTS error: " << msg.error_message << "\n";
            {
                std::lock_guard<std::mutex> lk(doneMutex);
                failed.store(true);
                done.store(true);
            }
            doneCv.notify_all();
            readyCv.notify_all();
        } break;
        default: {
            std::cerr << "Received unknown message type: " << msg.type_str << "\n";
        } break;
        };
        });

    client.setOnError([&](const std::string& err) {
        std::cerr << "WebSocket error: " << err << "\n";
        {
            std::lock_guard<std::mutex> lk(doneMutex);
            failed.store(true);
            done.store(true);
        }
        doneCv.notify_all();
        readyCv.notify_all();
        });

    client.setOnClosed([&] {
        {
            std::lock_guard<std::mutex> lk(doneMutex);
            if (!done.load()) {
                failed.store(true);
            }
            done.store(true);
        }
        doneCv.notify_all();
        readyCv.notify_all();
        });

    try {
        client.connect(apiKeyEnv);

        gradium::TtsRealtimeSetup setup;
        setup.voice_id = voiceId;
        setup.output_format = outputFormat;
        client.setup(setup);

        {
            std::unique_lock<std::mutex> lk(readyMutex);
            readyCv.wait(lk, [&] { return ready.load() || failed.load(); });
        }

        if (failed.load()) {
            throw std::runtime_error("[gradiumpp] TTS session ended before ready");
        }

        client.sendText(text);
        client.sendEndOfStream();

        {
            std::unique_lock<std::mutex> lk(doneMutex);
            doneCv.wait(lk, [&] { return done.load(); });
        }

        out.close();
        client.close();

        if (failed.load()) {
            std::cerr << "\nTTS realtime session did not complete successfully.\n";
            return 1;
        }

        std::cout << "Wrote realtime TTS output to " << outPath << "\n";
    }
    catch (const std::exception& ex) {
        std::cerr << "TTS realtime failed: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
