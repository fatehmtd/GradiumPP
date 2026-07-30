#include <CLI/CLI.hpp>
#include <gradiumpp/gradium.hpp>

#include <cppcodec/base64_rfc4648.hpp>

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
    std::string voiceId;
    std::string inputFormat  = "pcm";
    std::string outputFormat = "pcm_16000";
    std::string targetLanguage;
    std::string outPath = "s2s_realtime_output.pcm";

    CLI::App app{ "Transcribe, translate, and re-synthesize audio over the Gradium S2S WebSocket API" };
    app.add_option("--file", filePath, "Path to a raw PCM input file (24 kHz, 16-bit, mono)")->required();
    app.add_option("--voice", voiceId, "Voice ID for the synthesized output")->required();
    app.add_option("--input-format", inputFormat, "Input audio format");
    app.add_option("--output-format", outputFormat, "Output audio format");
    // The API's only model, "s2s-translate", rejects setup without a target_language.
    app.add_option("--target-language", targetLanguage, "Language to translate speech into (e.g. \"en\")")->required();
    app.add_option("--out", outPath, "Output file path for the synthesized audio");
    CLI11_PARSE(app, argc, argv);

    std::ifstream audioFile(filePath, std::ios::binary);
    if (!audioFile.is_open()) {
        std::cerr << "Error: cannot open audio file: " << filePath << "\n";
        return 1;
    }
    const std::vector<std::uint8_t> audioData(
        (std::istreambuf_iterator<char>(audioFile)),
        std::istreambuf_iterator<char>());

    std::ofstream out(outPath, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Error: cannot open output file: " << outPath << "\n";
        return 1;
    }

    std::atomic<bool>       ready{ false };
    std::mutex              readyMutex;
    std::condition_variable readyCv;

    std::atomic<bool>       done{ false };
    std::atomic<bool>       failed{ false };
    std::mutex              doneMutex;
    std::condition_variable doneCv;

    gradium::S2sRealtimeClient client;

    client.setOnParsedMessage([&](const gradium::S2sRealtimeMessage& msg) {
        switch (msg.type) {
        case gradium::S2sRealtimeMessage::Type::Ready:
        {
            {
                std::lock_guard<std::mutex> lk(readyMutex);
                ready.store(true);
            }
            readyCv.notify_all();
            std::cout << "S2S session is ready. Request ID: " << msg.request_id << "\n";
        }
        break;
        case gradium::S2sRealtimeMessage::Type::Text:
        {
            std::cout << "[text] " << msg.text
                << "  (" << msg.start_s << "s - " << msg.stop_s << "s)\n";
        }
        break;
        case gradium::S2sRealtimeMessage::Type::Audio:
        {
            if (!msg.audio_base64.empty()) {
                auto bytes = cppcodec::base64_rfc4648::decode(msg.audio_base64);
                out.write(reinterpret_cast<const char*>(bytes.data()),
                    static_cast<std::streamsize>(bytes.size()));
            }
        }
        break;
        case gradium::S2sRealtimeMessage::Type::EndOfStream:
        {
            {
                std::lock_guard<std::mutex> lk(doneMutex);
                done.store(true);
            }
            doneCv.notify_all();
        }
        break;
        case gradium::S2sRealtimeMessage::Type::Error:
        {
            // Wake the ready-wait too — an error can arrive before "ready".
            std::cerr << "S2S error: " << msg.error_message << "\n";
            {
                std::lock_guard<std::mutex> lk(doneMutex);
                failed.store(true);
                done.store(true);
            }
            doneCv.notify_all();
            readyCv.notify_all();
        }
        break;
        default:
            break;
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
        readyCv.notify_all();
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
        readyCv.notify_all();
        });

    try {
        gradium::S2sRealtimeSetup setup;
        setup.voice_id         = voiceId;
        setup.input_format     = inputFormat;
        setup.output_format    = outputFormat;
        setup.target_language  = targetLanguage;
        client.connect(apiKeyEnv, setup);

        {
            std::unique_lock<std::mutex> lk(readyMutex);
            readyCv.wait(lk, [&] { return ready.load() || failed.load(); });
        }

        if (failed.load()) {
            throw std::runtime_error("[gradiumpp] S2S session ended before ready");
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

        out.close();
        client.close();

        if (failed.load()) {
            std::cerr << "\nS2S session did not complete successfully.\n";
            return 1;
        }

        std::cout << "\nWrote realtime S2S output to " << outPath << "\n";
    }
    catch (const std::exception& ex) {
        std::cerr << "S2S realtime failed: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
